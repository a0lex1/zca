#pragma once

#include "co/async/thread_model.h"

namespace co {
namespace async {
namespace capsule {

// ExceptionModel try/catch-wraps every stage of running LoopObject(s)
// If exception caught, ExceptionModel decices whether it is recoverable or not
//
class ExceptionModel {
public:
  virtual ~ExceptionModel() = default;

  enum class ExceptionState {
    kNormalExit,
    kExceptionRecoverable,
    kExceptionUnrecoverable,
  };

  using ThreadModel = co::async::ThreadModel;

  virtual void DoWrappedInitialize(Func<void()> fn_init, ExceptionState& er) = 0; // not now
  virtual void DoWrappedPrepareToStart(Func<void()> fn_prep, ExceptionState& er) = 0;
  virtual void DoWrappedStart(Func<void()> fn_start, ExceptionState& er) = 0;
  virtual void DoWrappedRunThreadModel(ThreadModel& tm, ExceptionState& er) = 0;
};

// ------------------------------------------------------------------------------------------------------------------------------------

// BasicExceptionModel catches known <co::Exception>s plus the default
// exceptions like boost's and std::exception
// If you don't have your own exception model, use this
//
class BasicExceptionModel : public ExceptionModel {
public:
  virtual ~BasicExceptionModel() = default;

  using ThreadModel = co::async::ThreadModel;

  BasicExceptionModel(bool stop_on_exception);

  std::exception_ptr GetExitException();

private:
  // [OpaqueExceptionModel override]
  virtual void IoctxRunFunc(io_context& ioc);
  // All Wrappers use same GenericTryCatch by default, override this if u want
  void DoWrappedInitialize(Func<void()> fn_init, ExceptionState& er) override;
  void DoWrappedPrepareToStart(Func<void()> fn_prep, ExceptionState& er) override;
  void DoWrappedStart(Func<void()> fn_start, ExceptionState& er) override;
  void DoWrappedRunThreadModel(ThreadModel& tm, ExceptionState& er) override;

  // Overload this if you want your own exceptions
  void GenericTryCatch(Func<void()> cbk);;

protected:
  // For use inside IoctxRunFunc
  ThreadModel& GetCurrentThreadModel();
  // Proper12 stop from inside IoctxRunFunc()'s catch() block
  void OnExceptionWithExcpState(ExceptionState);
  // Proper12 stop from inside IoctxRunFunc()'s catch() block
  void RecoverableContinue();

public:
  bool stop_on_exception_;
  ThreadModel* cur_tm_{ nullptr };
  IoctxRunnerFromFunc ioctx_runner_;
  ExceptionState exit_result_{ ExceptionState::kNormalExit };
  std::exception_ptr exit_exception_;
  boost::mutex stop_mutex_;
  boost::thread::id stop_tid_;
};


}}}


