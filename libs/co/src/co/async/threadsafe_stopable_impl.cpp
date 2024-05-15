#include "co/async/threadsafe_stopable_impl.h"
#include "co/xlog/xlog.h"

#include <boost/stacktrace.hpp>

#ifdef __APPLE__
#define _GNU_SOURCE
#endif

#include <boost/thread/thread.hpp>

using namespace std;

namespace co {
namespace async {

ThreadsafeStopableImpl::ThreadsafeStopableImpl(Stopable& stopable,
                                               Shptr<Strand> strand)
  : user_stopable_(stopable), strand_(strand)
{

}

ThreadsafeStopableImpl::~ThreadsafeStopableImpl() {
  DCHECK(!(_dbg_stopunsafe_call_count_ > _dbg_beforestart_called_));
  if (_dbg_beforestart_called_ != _dbg_stopunsafe_call_count_) {
    // We're stopped in stop-ioc mode (interrupted)
  }
}

void ThreadsafeStopableImpl::StopThreadsafe()
{
  _dbg_stopthreadsafe_called_ += 1;

  auto grabd_ref(InterlockedGrabLastRef());
  if (grabd_ref == nullptr) {
    // other thread / not started
    return;
  }
  // Either this piece of code is executed or OnIoEnded()'s piece
  bools_.executing_stopthreadsafe_block = true;

  // LOCATION STACKTRACE
  //auto st = boost::stacktrace::stacktrace(); // TODO: REMOVE
  boost::asio::post(*strand_.get(), [&, grabd_ref/*keeping-alive*/]() {
    DCHECK(strand_->running_in_this_thread());
    //syslog(_DBG) << "#! StopThreadsafe CALLBACK (inside fiber)\n";

    // We got stop before start. It's ok, nothing to do.
    if (!(bools_.beforestarted_)) {
      return;
    }

    _dbg_stopunsafe_call_count_ += 1;
    user_stopable_.StopUnsafe();

    bools_.stopped_ = true;
    /* |grabbed_ref| released after return */
                    });
}

void ThreadsafeStopableImpl::BeforeStart(RefTracker rt) {
  DCHECK(!bools_.beforestarted_);
  DCHECK(!bools_.threadsafe_starting_);
  DCHECK(!bools_.threadsafe_started_);
  DCHECK(!bools_.executing_ioended_block_);
  DCHECK(!bools_.executing_stopthreadsafe_block);

  // make |rt| copy to |last_ref_| and insert i/o ended callback to |rt|
  last_ref_ = boost::make_shared<RefTracker>(rt);
  bools_.beforestarted_ = true;
  _dbg_beforestart_called_ += 1;
}

void ThreadsafeStopableImpl::BeforeStartWithIoEnded(RefTracker rt,
                                                    RefTracker& rt_new) {
  DCHECK(!bools_.beforestarted_);
  DCHECK(!bools_.threadsafe_starting_);
  DCHECK(!bools_.threadsafe_started_);

  // make |rt| copy to |last_ref_| and insert i/o ended callback to |rt|
  last_ref_ = boost::make_shared<RefTracker>(rt);

  bools_.beforestarted_ = true;
  _dbg_beforestart_called_ += 1;

  rt_new = RefTracker(CUR_LOC(),
                    [&] () {
      OnIoEnded();
    },
    rt/*last_ref_*/);
}

void ThreadsafeStopableImpl::DoThreadsafeStart(Func<void()> start_fn) {
  DCHECK(!bools_.threadsafe_starting_);
  DCHECK(!bools_.threadsafe_started_);

  auto cur_tid(boost::this_thread::get_id());

  bools_.threadsafe_starting_ = true;

  strand_->dispatch([&, start_fn, cur_tid] () {
    // inside unknown context

    start_fn();

    bools_.threadsafe_started_ = true;
  });
}

void ThreadsafeStopableImpl::OnIoEnded() {
  // inside unknown fiber

  // Grab last ref.
  // Following code is executed once (although we are in unknown fiber)
  // we're in i/o end cbk (=> no more async ops will be inited) AND` we've grabbed
  // last ref so there will be no another parallel Stop()s, it's safe to Stop() here.
  auto grabd_ref(InterlockedGrabLastRef());
  if (grabd_ref == nullptr) {
    // other thread / not started
    return;
  }

  _dbg_stopunsafe_call_count_ += 1;
  bools_.executing_ioended_block_ = true;
  //bools_.calling_stopunsafe_ = true;

  //
  // call user's StopUnsafe() implementation
  //
  user_stopable_.StopUnsafe();

  //bools_.stopunsafe_returned_ = true;
}

Shptr<co::RefTracker> ThreadsafeStopableImpl::InterlockedLoadLastRef() {
  // inside unknown fiber
  return co::to_std(last_ref_.load());
}

Shptr<co::RefTracker> ThreadsafeStopableImpl::InterlockedGrabLastRef() {
  // inside unknown fiber
  boost::shared_ptr<RefTracker> somert(nullptr);
  boost::shared_ptr<RefTracker> was = last_ref_.exchange(somert);
  if (was != nullptr) {
    Shptr<RefTracker> stdref(co::to_std<RefTracker>(was));
    return stdref;
  }
  return nullptr;
}

}}

