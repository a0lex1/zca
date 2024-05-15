#pragma once

#include "co/async/capsule/capsule_object_factory.h"
#include "co/async/capsule/run_loop.h"

#include "co/async/startable_stopable.h"
#include <boost/throw_exception.hpp>

namespace co {
namespace async {
namespace capsule {

/*
* class Capsule
* 
*/
class Capsule : public co::async::ThreadsafeStopable {
public:
  virtual ~Capsule() = default;

  Capsule(CapsuleObjectFactory& objfac) : objfac_(objfac) {}

  using ThreadModel = co::async::ThreadModel;
  using ExitResult = ExceptionModel::ExceptionState;

  // available after Initialize(), collected in Run()
  RunLoopCounters GetRunLoopCounters() const { return run_loop_->GetCounters(); }

  void Initialize(ExitResult& er);
  void Run(RunLoop::LoopExitCause& loop_exit_cause,
           ExceptionModel::ExceptionState& last_excp_state);

  // how many after Run()
  size_t RefTrackersLeft() const;

  void Reset();
  void StopThreadModel();
  void StopThreadsafe() override;

  // exception throwing wrappers
  void Initialize();
  void Run();

protected:
  ThreadModel& GetThreadModel() { return *thread_model_.get(); }
  FailsafeModel& GetFailsafeModel() { return *failsafe_model_; }
  LoopObject& GetIoInitiator() { return *io_initiator_; }
  Cooldowns& GetCooldowns() { return *cooldowns_; }

private:
  void InitializeWithexcp();

private:
  CapsuleObjectFactory& objfac_;
  Uptr<Restarter> restart_logic_;
  Uptr<FailsafeModel> failsafe_model_;
  Uptr<ThreadModel> thread_model_;
  Uptr<RunLoop> run_loop_;
  Shptr<LoopObject> io_initiator_;
  Uptr<Cooldowns> cooldowns_;
  Uptr<LoopOptions> loop_options_;
  bool initialize_done_{ false };
  bool run_done_{ false };
};


}}}

