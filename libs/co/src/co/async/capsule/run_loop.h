#pragma once

#include "co/async/capsule/run_loop_counters.h"
#include "co/async/capsule/loop_options.h"
#include "co/async/capsule/restarter.h"
#include "co/async/capsule/exception_model.h"
#include "co/async/capsule/cooldowns.h"

#include "co/async/loop_object.h"
#include "co/async/thread_model.h"

#include "co/xlog/define_logger_sink.h"

#include <atomic>

namespace co {
namespace async {
namespace capsule {

DECLARE_XLOGGER_SINK("runloop", gCoAsyncCapsuleRunLoopSink);

class RunLoop : public co::async::ThreadsafeStopable {
public:
  virtual ~RunLoop() = default;

  using ThreadModel = co::async::ThreadModel;
  using RefTrackerContext = co::RefTrackerContext;

  // enum LoopExitCause
  //
  // determines logical block that `break`ed run loop
  // kUnrecoverableExceptionCause overrides kExitFlagCause if exit flag has been used
  enum class LoopExitCause {
    kExitFlagCause,
    kIterLimitReachedCause,
    kUnrecoverableExceptionCause
  };

  RunLoop(Restarter& restart_logic,
          ExceptionModel& em,
          Uptr<ThreadModel>& tm,
          Cooldowns& cooldowns,
          Shptr<LoopObject>& io_initiator,
          const LoopOptions& loop_options);

  // GetRefTrackerContext() functionality is not used, but it works.
  // RefTrackerContext is accessible before loop Run()s. At the end of Run()
  // it's deleted and new object created. So don't use it after Run().
  // Not threadsafe.
  RefTrackerContext& GetRefTrackerContext() { return *rtctx_.get(); }

  // Stop thread model.
  // NOTE: if SetExitFlag() is not set, restart may occur.
  void StopThreadsafe() override;

  // if |loop_exit_cause| != kUnrecoverableExceptionCause, |last_excp_state|
  // contains the most recent exception before loop exit flag set&detected
  // otherwise, |last_excp_state| determines the category of exception which
  // caused loop exit
  void Run(LoopExitCause& loop_exit_cause,
           ExceptionModel::ExceptionState& last_excp_state);

  // how many after Run()
  size_t RefTrackersLeft() const { return reftrackers_left_; }

  // thread safe
  // loop will exit after current operation is done (link, initiate, run)
  void SetExitFlag();

  // IsExitedFromException() returns true if exit flag is set & detected
  // _after_ exception-resulting call to Link, Initiate or Run stages
  // otherwise, exit_flag_ detected _between_ stages
  // We need to declare this variable explicitly, otherwise the
  // control over logic is lost
  bool IsExitedFromException() const { return exited_from_exception_; }

  RunLoopCounters GetCounters() const;

private:
  Uptr<RefTrackerContext> rtctx_;
  Restarter& restart_logic_;
  ExceptionModel& em_;
  Uptr<ThreadModel>& tm_;
  Cooldowns& cooldowns_;
  Shptr<LoopObject>& io_initiator_; // Shptr<>&
  const LoopOptions& loop_options_;
  std::atomic<bool> exit_flag_{ false };
  std::atomic<bool> exited_from_exception_{ false };
  size_t recovered_inits_{ 0 };
  size_t recovered_starts_{ 0 };
  size_t recovered_runs_{ 0 };
  size_t normal_exits_{ 0 };
  size_t cur_iter_{ 0 };
  size_t reftrackers_left_{ 0 }; // set after DoWrappedRunThreadModel stage
};

}}}



