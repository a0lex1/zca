#include "co/async/capsule/exception_model.h"
#include "co/base/print_diagnostic_information.h"
#include "co/base/recoverable_exception.h"
#include "co/base/unrecoverable_exception.h"
#include "co/base/config.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;

namespace co {
namespace async {
namespace capsule {

BasicExceptionModel::BasicExceptionModel(bool stop_on_exception)
  :
  stop_on_exception_(stop_on_exception),
  ioctx_runner_(co::bind(&BasicExceptionModel::IoctxRunFunc, this, _1))
{

}

std::exception_ptr BasicExceptionModel::GetExitException()
{
  return exit_exception_;
}


ThreadModel& BasicExceptionModel::GetCurrentThreadModel()
{
  // Must be used only from IoctxRunFunc
  DCHECK(cur_tm_ != nullptr);
  return *cur_tm_;
}

void BasicExceptionModel::OnExceptionWithExcpState(ExceptionState es)
{
  stop_mutex_.lock();
  if (!exit_exception_) {
    exit_exception_ = std::current_exception();
    stop_tid_ = boost::this_thread::get_id();
    exit_result_ = es;
  }
  stop_mutex_.unlock();

  if (cur_tm_ != nullptr) {
    cur_tm_->StopThreadsafe();
  }
}

void BasicExceptionModel::GenericTryCatch(Func<void()> cbk)
{
  static const auto print_xxx = []() { syslog(_ERR) << string(50, '#') << "\n"; };
  bool ok = false;
  bool unrecov_ = false;
  try {
    cbk();
    ok = true;
  }
  catch (RecoverableException& e) {
    print_xxx();
    syslog(_ERR) << "### RecoverableException: " << e.what() << "\n";
    print_xxx();
    unrecov_ = false;
    PrintBoostDiagnosticInformationXlog(e);
  }
  catch (UnrecoverableException& e) {
    print_xxx();
    syslog(_ERR) << "### UnrecoverableException: " << e.what() << "\n";
    print_xxx();
    unrecov_ = true;
    PrintBoostDiagnosticInformationXlog(e);
  }
  if (!ok) {
    if (!GetExitException()) { //rcond
      // If |stop_on_exception_|, aways break. If not, always continue.
      if (unrecov_) {
        syslog(_INFO) << "*STOPPING* (stop_on_exception_==true)\n";
        // Here, in BasicExceptionModel we define all exceptions as unrecoverable
        OnExceptionWithExcpState(ExceptionState::kExceptionUnrecoverable);
      }
      else {
        syslog(_INFO) << "*EXCEPTION-CONTINUING*\n";
        OnExceptionWithExcpState(ExceptionState::kExceptionRecoverable);
      }
    }
    else {
      // exit exception was already saved, don't save it again
      syslog(_WARN) << "**LOOOL** exception during stopping-due-to-exception, go check brain\n";
    }
  }
}

void BasicExceptionModel::IoctxRunFunc(io_context& ioc)
{
  GenericTryCatch([&]() {
    ioc.run();
                  });
}

void BasicExceptionModel::DoWrappedInitialize(Func<void()> fn_init,
                                              ExceptionState& er)
{
  exit_result_ = ExceptionState::kNormalExit;

  GenericTryCatch([&]() {
    fn_init();
                  });

  er = exit_result_;
}

void BasicExceptionModel::DoWrappedPrepareToStart(Func<void()> fn_prep,
                                                  ExceptionState& er)
{
  exit_result_ = ExceptionState::kNormalExit;

  GenericTryCatch([&]() {
    fn_prep();
                  });

  er = exit_result_;
}

void BasicExceptionModel::DoWrappedStart(Func<void()> fn_start, ExceptionState& er)
{
  exit_result_ = ExceptionState::kNormalExit;

  GenericTryCatch([&]() {
    fn_start();
                  });
  er = exit_result_;
}

void BasicExceptionModel::DoWrappedRunThreadModel(ThreadModel& tm, ExceptionState& er)
{
  exit_result_ = ExceptionState::kNormalExit;
  cur_tm_ = &tm;

  GenericTryCatch([&]() {
    tm.Run(ioctx_runner_/*ourself*/);
                  });

  cur_tm_ = nullptr;
  er = exit_result_;
}

}}}


