#include "co/async/test_kit/default_capsule_object_tester.h"

#include "co/async/capsule/capsule_builder.h"
#include "co/async/configs/thread_model_config_from_dict.h"

#include "co/xlog/xlog.h"

#include <sstream>

using namespace std;

namespace co {
namespace async {
namespace test_kit {

DefaultCapsuleObjectTester::DefaultCapsuleObjectTester(
  TestInfo& ti,
  Func<Uptr<co::async::LoopObject>(co::async::ThreadModel&)> io_initiator_creator_func,
  size_t max_total_iterations,
  size_t max_normal_exit_iters,
  bool threat_aborted_stop_error,
  Func<Uptr<Cooldowns>()> fn_createcooldowns)
  :
  ti_(ti),
  threat_aborted_stop_error_(threat_aborted_stop_error)
{
  // We don't create tm_conf_ here because it can throw (we don't want to throw from CTOR)
  // Otherwise, we create tm_conf_ in execute. Here we're passing only a reference.
  builder_ = make_unique<capsule::CapsuleBuilder>();
  builder_->SetCooldownsCreator(fn_createcooldowns);
  builder_
    ->SetIoInitiatorCreator(io_initiator_creator_func)
    .SetLoopOptionsCreator([=]() {
    return make_unique<capsule::LoopOptions>(
      max_total_iterations,
      max_normal_exit_iters);
      })
    .SetThreadModelCreator([&]() {
        return make_unique<ThreadModel>(tm_conf_);
      });
}

void DefaultCapsuleObjectTester::Execute(
  LoopExitCause& exit_cause,
  ExceptionState& excp_state)
{
  tm_conf_ = configs::ThreadModelConfigFromDict(ThreadModelConfig(), ti_.opts_dict, ConsumeAction::kDontConsume);

  capsule_ = make_unique<capsule::Capsule>(builder_->GetObjectFactory());

  syslog(_DBG) << "Doing capsule.Initialize()...\n";
  capsule_->Initialize();

  syslog(_DBG) << "Doing capsule.Run()...\n";

  capsule_->Run(exit_cause, excp_state);

  if (threat_aborted_stop_error_) {
    // after last Run()
    if (capsule_->RefTrackersLeft() != 0) {
      syslog(_FATAL) << "Returned from capsule.Run. !!!RefTrackers left!!!:\n";

      stringstream ss;
      PrintRefTrackerContextDataList(ss);
      syslog(_FATAL) << ss.str() << "\n";

      abort();
    }
    else {
      syslog(_DBG) << "Returned from capsule.Run. VERIFIED: 0 RefTrackers after Run (OK)\n";
    }
  }
  else {
    syslog(_DBG) << "Returned from capsule.Run. Not asked to verify RefTrackers after Run.\n";
  }
}

void DefaultCapsuleObjectTester::ExecuteExpect(LoopExitCause expected_exit_cause,
  ExceptionState expected_excp_state)
{
  LoopExitCause exit_cause;
  ExceptionState excp_state;

  Execute(exit_cause, excp_state);

  DCHECK(exit_cause == expected_exit_cause);
  DCHECK(excp_state == expected_excp_state);

}

}}}

