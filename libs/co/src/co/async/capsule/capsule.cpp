#include "co/async/capsule/capsule.h"

#include "co/xlog/xlog.h"

namespace co {
namespace async {
namespace capsule {

void Capsule::Initialize(ExitResult& er)
{
  DCHECK(!initialize_done_);
  failsafe_model_ = objfac_.CreateFailsafeModel();
  failsafe_model_->GetExceptionModel().DoWrappedInitialize(
    co::bind(&Capsule::InitializeWithexcp, this),
    er);
  if (er != ExitResult::kNormalExit) {
    return;
  }
  initialize_done_ = true;
}

void Capsule::Initialize()
{
  ExitResult er;
  Initialize(er);
  if (er != ExitResult::kNormalExit) {
    syslog(_DBG) << "Capsule::Link failed, er " << (int)er << "\n";
    BOOST_THROW_EXCEPTION(std::runtime_error("You failed. As a person. Error 255."));
  }
}

void Capsule::Run(RunLoop::LoopExitCause& loop_exit_cause, ExceptionModel::ExceptionState& last_excp_state)
{
  DCHECK(!run_done_);
  DCHECK(initialize_done_);
  syslog(_DBG) << "Doing run_loop_->Run()\n";

  // run_loop_ does initiate
  run_loop_->Run(loop_exit_cause, last_excp_state);

  syslog(_DBG) << "run_loop_->Run() returned exit cause " << (int)loop_exit_cause
    << ", excp state " << (int)last_excp_state << "\n";
  run_done_ = true;
}

size_t Capsule::RefTrackersLeft() const {
  return run_loop_->RefTrackersLeft();
}

void Capsule::InitializeWithexcp()
{
  // inside ExceptionModel's try{}
  restart_logic_ = objfac_.CreateRestarter();
  thread_model_ = objfac_.CreateThreadModel();
  io_initiator_ = objfac_.CreateIoInitiator(*thread_model_.get());
  cooldowns_ = objfac_.CreateCooldowns();
  loop_options_ = objfac_.CreateLoopOptions();
  run_loop_ = make_unique<RunLoop>(*restart_logic_.get(),
                                   failsafe_model_->GetExceptionModel(),
                                   thread_model_, // Uptr<>&
                                   *cooldowns_.get(),
                                   io_initiator_, // Uptr<>&
                                   *loop_options_.get()
                                   );
}

void Capsule::Run()
{
  RunLoop::LoopExitCause loop_exit_cause;
  ExceptionModel::ExceptionState last_excp_state;
  Run(loop_exit_cause, last_excp_state);
  if (last_excp_state != ExitResult::kNormalExit) {
    syslog(_DBG) << "Capsule::Run failed, exit cause " << (int)loop_exit_cause << "\n";
    BOOST_THROW_EXCEPTION(std::runtime_error("Cp:R f"));
  }
}

void Capsule::Reset()
{
  // to next run
  loop_options_ = nullptr;
  cooldowns_ = nullptr;
  io_initiator_ = nullptr;
  run_loop_ = nullptr;
  failsafe_model_ = nullptr;
  thread_model_ = nullptr; // the last one
  //
  initialize_done_ = false;
  run_done_ = false;
}

void Capsule::StopThreadModel()
{
  thread_model_->StopThreadsafe();
  // they will manage aborted stop cleanup
}

void Capsule::StopThreadsafe()
{
  io_initiator_->StopThreadsafe();
}

} } }

