#pragma once

#include "co/async/startable_stopable.h"
#include "co/base/ref_tracker.h"

#include <boost/asio/strand.hpp>
#include <boost/make_shared.hpp>
#include <boost/smart_ptr/atomic_shared_ptr.hpp>

#include <sstream>
#include <atomic>

namespace co {
namespace async {

/*
* class ThreadsafeStopableImpl
* 
* Thread-safe stop functionality implementation
* 
* Works through |last_ref_|
* 
* You (derived class) bring him StopUnsafe() and get your own StopThreadsafe() impl
* 
* Your object must be single-fibered because he's gonna call your StopUnsafe() in that fiber
* If there are another fibers, accessing data being closed by you, it's a racecond
* like read-after-close, write-after-close
* Classes that manager other thread-safe stopable objects shouldn't ......?
* 
*/

class ThreadsafeStopableImpl : public ThreadsafeStopable {
public:
  virtual ~ThreadsafeStopableImpl();
  
  ThreadsafeStopableImpl(Stopable& stopable, Shptr<Strand> strand);

  // Why isn't OnIoEnded() hidden and auto-called? User code may need to
  // insert code before OR after OnIoEnded (that calls StopUnsafe).
  // If you want simplier things, see BeforeStartWithIoEnded.
  void BeforeStart(RefTracker rt);
  void OnIoEnded();
  void StopThreadsafe() override;
  Shptr<RefTracker> InterlockedLoadLastRef();

  // More simple variant of BeforeStart, adds IoEnded call to |rt| chain.
  void BeforeStartWithIoEnded(RefTracker rt, RefTracker& rt_new);

  Strand& GetStrand() { return *strand_.get(); }
  Shptr<Strand> GetStrandShptr() { return strand_; }

  // Optional to use.
  // |start_fn| must be bound with RefTracker
  void DoThreadsafeStart(Func<void()> start_fn);

private:
  Shptr<RefTracker> InterlockedGrabLastRef();

private:
  struct {
    bool threadsafe_starting_  : 1;
    bool threadsafe_started_ : 1;
    bool beforestarted_ : 1;
    bool posting_stop_ : 1;
    bool stopped_ : 1;
    bool executing_ioended_block_ : 1;
    bool executing_stopthreadsafe_block : 1;
    //bool calling_stopunsafe_ : 1;
    //bool stopunsafe_returned_ : 1;
  } bools_ { false, false, false, false, false, false, false };

  Stopable& user_stopable_;
  Shptr<Strand> strand_;
  AtomicShptr<RefTracker> last_ref_;

  std::atomic<size_t> _dbg_beforestart_called_{ 0 };
  std::atomic<size_t> _dbg_stopthreadsafe_called_{ 0 };
  size_t _dbg_stopunsafe_call_count_{ 0 };

};


}}

