#include "co/async/capsule/run_loop.h"

#include "co/xlog/xlog.h"

namespace co {
namespace async {
namespace capsule {

DEFINE_XLOGGER_SINK("runloop", gCoAsyncCapsuleRunLoopSink);
#define XLOG_CURRENT_SINK gCoAsyncCapsuleRunLoopSink

RunLoop::RunLoop(Restarter& restart_logic,
                 ExceptionModel& em,
                 Uptr<ThreadModel>& tm,
                 Cooldowns& cooldowns,
                 Shptr<LoopObject>& io_initiator,
                 const LoopOptions& loop_options)
  :
  restart_logic_(restart_logic),
  em_(em),
  tm_(tm),
  cooldowns_(cooldowns),
  io_initiator_(io_initiator),
  loop_options_(loop_options),
  rtctx_(make_unique<RefTrackerContext>(CUR_LOC()))
{
  // |rtctx_| is never nullptr, but it's recreated after each Run().
}

void RunLoop::StopThreadsafe() {
  tm_->StopThreadsafe();
}

RunLoopCounters RunLoop::GetCounters() const {
  RunLoopCounters ret;
  ret.normal_exits = normal_exits_;
  ret.cur_iter = cur_iter_;
  ret.recovered_inits = recovered_inits_;
  ret.recovered_starts = recovered_starts_;
  ret.recovered_runs = recovered_runs_;
  return ret;
}

void RunLoop::Run(LoopExitCause& loop_exit_cause,
                  ExceptionModel::ExceptionState& last_excp_state)
{
  using ExitResult = ExceptionModel::ExceptionState;

  exit_flag_ = false;
  exited_from_exception_ = false;

  reftrackers_left_ = 0;

  const char* break_reason = nullptr;

  // now go loop
  for (cur_iter_ = 0; ; cur_iter_++) {
    // CHECK EXIT FLAG
    if (exit_flag_ == true) {
      goto ExitFlag;
    }

    if (loop_options_.GetMaxTotalIterations() != LoopOptionsBase::kUnlimitedIterations) {
      if (cur_iter_ == loop_options_.GetMaxTotalIterations()) {
        Log(_DBG) << "[RunLoop] BEFORE INIT PHASE (begin iter), LIMIT REACHED <total iters>\n";
        break_reason = "total_before";
        break;
      }
    }
    if (loop_options_.GetMaxNormalExitIterations() != LoopOptionsBase::kUnlimitedIterations) {
      if (normal_exits_ == loop_options_.GetMaxNormalExitIterations()) {
        // this if() would be checked again before entering next iteration
        Log(_DBG) << "[RunLoop] BEFORE INIT PHASE (begin iter), LIMIT REACHED <normal iters>\n";
        break_reason = "total_normalexit_before";
        break;
      }
    }

    // ------------------------------------------------------------------------------------------------------------------------------
    // INSIDE LOOP - <PREPARE-TO-START> PHASE
    // ------------------------------------------------------------------------------------------------------------------------------
    last_excp_state = ExceptionModel::ExceptionState::kNormalExit;
    em_.DoWrappedInitialize([&]() {
      Errcode e;
      io_initiator_->PrepareToStart(e);
      if (e) {
        Log(_ERR) << "io_initiator_->PrepareToStart() failed, err " << e << ", throwing boost::system::system_error exception ...\n";
        // Throw PrepareToStart() error as exception
        // Will be caught by the next block of code
        BOOST_THROW_EXCEPTION(boost::system::system_error(e));
      }
                      },
                      last_excp_state);

    // First check if exception occurred
    switch (last_excp_state) {
    case ExitResult::kNormalExit:
      Log(_DBG) << "[RunLoop] Prepare-to-start phase success\n";
      break;

    case ExitResult::kExceptionRecoverable:
      // CHECK EXIT FLAG
      if (exit_flag_ == true) {
        goto ExitFlagFromException;
      }
      Log(_ERR) << "[RunLoop] Prepare-to-start phase: Caught recoverable exception, cooldown...\n";
      cooldowns_.CooldownBeforeRestart(Cooldowns::WhatFor::kInitPhaseException);

      Log(_INFO) << "[RunLoop] Prepare-to-start phase: recovering after exception\n";
      restart_logic_.DoRestartAfterInitException(tm_, io_initiator_);
      recovered_inits_ += 1;
      Log(_INFO) << "[RunLoop] Prepare-to-start phase: recovered (#" << recovered_inits_ << " inits)\n";

      continue; // |exit_flag_| will be checked

    case ExitResult::kExceptionUnrecoverable:
      Log(_DBG) << "[RunLoop] Prepare-to-start phase *Unrecoverable* exception, exiting\n";
      goto UnrecovExit;

    default:
      NOTREACHED();
    }

    // CHECK EXIT FLAG
    if (exit_flag_ == true) {
      goto ExitFlag;
    }

    // ------------------------------------------------------------------------------------------------------------------------------
    // INSIDE LOOP - <START> PHASE
    // ------------------------------------------------------------------------------------------------------------------------------

    // All the reftrackers of this iteration will be bound to |rtctx|

    Log(_INFO) << "[RunLoop] Doing DoWrappedStart()\n";

    em_.DoWrappedStart(
                       [&]() {
      Log(_DBG) << "[RunLoop] Inside excp.model's try/catch, Start()ing\n";

      io_initiator_->Start(RefTracker(CUR_LOC(), rtctx_->GetHandle(),
                       []() {
        Log(_INFO) << "[RunLoop] IoInitiator #ioended\n";
                           }));
                       },
                       last_excp_state);

    switch (last_excp_state) {
      // START PHASE - NORMAL EXIT
    case ExitResult::kNormalExit:
      Log(_DBG) << "[RunLoop] Start phase success\n";
      break;

    // START PHASE - RECOVERABLE
    case ExitResult::kExceptionRecoverable:
      // CHECK EXIT FLAG
      if (exit_flag_ == true) {
        goto ExitFlagFromException;
      }
      Log(_ERR) << "[RunLoop] Start phase: Caught recoverable exception, cooldown...\n";
      cooldowns_.CooldownBeforeRestart(Cooldowns::WhatFor::kStartPhaseException);

      Log(_INFO) << "[RunLoop] Start phase: recovering after exception\n";
      restart_logic_.DoRestartAfterInitException(tm_, io_initiator_);
      recovered_starts_ += 1;
      Log(_INFO) << "[RunLoop] Start phase: recovered (#" << recovered_starts_ << ")\n";

      continue;// |exit_flag_| will be checked

      // START PHASE - UNRECOVERABLE
    case ExitResult::kExceptionUnrecoverable:
      Log(_DBG) << "[RunLoop] Start phase *Unrecoverable* exception, exiting\n";
      goto UnrecovExit;

    default:
      NOTREACHED();
    }

    // CHECK EXIT FLAG _after_ dealing with exception state
    if (exit_flag_ == true) {
      goto ExitFlag;
    }

    // ------------------------------------------------------------------------------------------------------------------------------
    // INSIDE LOOP - <RUN> PHASE ******************************************************************************************
    // ------------------------------------------------------------------------------------------------------------------------------
    Log(_DBG) << "[RunLoop] Doing DoWrappedRun()\n";

    em_.DoWrappedRunThreadModel(*tm_.get(), last_excp_state);

    switch (last_excp_state) {
    // RUN PHASE - NORMAL EXIT
    case ExitResult::kNormalExit:
      reftrackers_left_ = rtctx_->GetAtomicRefTrackerCount();
      if (reftrackers_left_ != 0) {        

        // --------------------------------------------------------------------------------------------------
        // we have some RefTrackers left, cleanup!
        // --------------------------------------------------------------------------------------------------
        Log(_DBG) << "[RunLoop] +++++++++++++++++++++++++++++++++++++++++++++++++\n"
                  << "[RunLoop] RUN PHASE, NORMAL EXIT, RTs LEFT!!!( " << rtctx_->GetAtomicRefTrackerCount() << " rts)\n"
                  << "[RunLoop] +++++++++++++++++++++++++++++++++++++++++++++++++\n";

        std::stringstream rtlist_text;

        // #Spoil
        PrintRefTrackerContextDataList(rtlist_text);

        rtctx_->DisableOnReleaseCalls();
        io_initiator_->CleanupAbortedStop();
      }
      else {
        // --------------------------------------------------------------------------------------------------
        // no RefTrackers means we won't need to cleanup after aborted stop
        // if there would be objects that require cleanup, links to them
        // would be in root RefTracker
        // --------------------------------------------------------------------------------------------------
        Log(_DBG) << "[RunLoop] RUN PHASE, NORMAL EXIT, CLEAN RefTrackerContext\n";
      }
      break;

      /*
      * IMPORTANT thing about Restarter
      * It's important to bool recreate_thread_model = true, otherwise we'll can get
      * some Server's Session(s) posted their StopThreadsafe() AFTER restarting.
      * E.g. Session(s) from previous run iter are completing on current run iter.
      */
    // RUN PHASE - RECOVERABLE
    case ExitResult::kExceptionRecoverable:
      // CHECK EXIT FLAG
      if (exit_flag_ == true) {
        goto ExitFlagFromException;
      }
      Log(_DBG) << "[RunLoop] RUN PHASE, RECOVERABLE EXCP, COOLING ...\n";
      cooldowns_.CooldownBeforeRestart(Cooldowns::WhatFor::kRunPhaseException);

      Log(_INFO) << "[RunLoop] RUN PHASE: RECOVERING AFTER EXCP\n";
      rtctx_->DisableOnReleaseCalls();
      io_initiator_->CleanupAbortedStop();
      restart_logic_.DoRestartAfterRunException(tm_, io_initiator_); //! Uptr<>&
      recovered_runs_ += 1;
      Log(_INFO) << "[RunLoop] RUN PHASE: RECOVERED (" << recovered_runs_ << " times recovered)\n";

      continue; // |exit_flag_| will be checked
      // RUN PHASE - UNRECOVERABLE

    case ExitResult::kExceptionUnrecoverable:
      Log(_INFO) << "[RunLoop] RUN PHASE: ***Unrecoverable*** EXCEPTION, DOING CLEANUP AND EXITING\n";
      rtctx_->DisableOnReleaseCalls();
      io_initiator_->CleanupAbortedStop();
      goto UnrecovExit;
    }

    // if we here, this must be true
    DCHECK(last_excp_state == ExitResult::kNormalExit);
    normal_exits_ += 1;

    if (loop_options_.GetMaxNormalExitIterations() != LoopOptionsBase::kUnlimitedIterations) {
      if (normal_exits_ == loop_options_.GetMaxNormalExitIterations()) {
        // we check |normal_exits_| at the beginning of loop and here,
        // before continue to the next iteration
        // this second check is required because we don't want to cooldown
        // here, we need to exit right now
        Log(_DBG) << "[RunLoop] AFTER RUN PHASE, LIMIT REACHED <normal iters> (check#2)\n";
        // break will lead us to returning kIterLimitReachedCause
        break_reason = "total_normalexit_after";
        break;
      }
    }
    if (exit_flag_ == true) {
      goto ExitFlag;
    }

    // NORMAL EXIT -> RESTART
    Log(_DBG) << "[RunLoop] -= AFTER RUN PHASE, NORMAL EXIT RESTARTING =- (" << normal_exits_ << " times restarted). Cooling\n";
    cooldowns_.CooldownBeforeRestart(Cooldowns::WhatFor::kNormalExit);

    // ------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // Will do io_initiator_.ResetToNextRun()
    // ------------------------------------------------------------------------------------------------------------------------------------------------------------------
    restart_logic_.DoRestartAfterNormalExit(tm_, io_initiator_);

    Log(_DBG) << "[RunLoop] next restart iteration...\n";

  } // for()
  // ourside for(), limit reached (GetMaxTotalIterations())
  loop_exit_cause = LoopExitCause::kIterLimitReachedCause;
  Log(_INFO) << "[RunLoop] Exiting run loop, cause: iter limit (break reason: " << break_reason << ")\n";
  goto CleanupReturn;

ExitFlag:
  loop_exit_cause = LoopExitCause::kExitFlagCause;
  Log(_INFO) << "[RunLoop] Exiting run loop, cause: xit flag was set\n";
  goto CleanupReturn;

UnrecovExit: { }
  loop_exit_cause = LoopExitCause::kUnrecoverableExceptionCause;
  Log(_INFO) << "[RunLoop] Exiting run loop, cause: unrecoverable exception occurred\n";
  goto CleanupReturn;

ExitFlagFromException:
  loop_exit_cause = LoopExitCause::kExitFlagCause;
  exited_from_exception_ = true;
  goto CleanupReturn;

CleanupReturn:
  // Reset |rtctx_|, we don't want memleaks
  rtctx_ = make_unique<RefTrackerContext>(CUR_LOC());

  // Added after fixing bug where RefTracker was inside coro. It wasn't released.
  Log(_DBG) << "[RunLoop] deleting io_initiator_\n";
  io_initiator_ = nullptr;
}

void RunLoop::SetExitFlag()
{
  exit_flag_ = true;
}

}}}

// goto is underrated.


