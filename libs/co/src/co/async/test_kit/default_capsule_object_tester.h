#pragma once

#include "co/async/capsule/capsule_builder.h"
#include "co/async/capsule/cooldowns.h"
#include "co/async/thread_model.h"
#include "co/async/loop_object.h"
#include "co/base/tests.h"

namespace co {
namespace async {
namespace test_kit {

class DefaultCapsuleObjectTester {
public:
  using LoopExitCause = capsule::RunLoop::LoopExitCause;
  using ExceptionState = capsule::ExceptionModel::ExceptionState;
  using Cooldowns = capsule::Cooldowns;

  DefaultCapsuleObjectTester(
    TestInfo& ti,
    Func<Uptr<co::async::LoopObject>(co::async::ThreadModel&)> io_initiator_creator_func,
    size_t max_total_iterations = 1,
    size_t max_normal_exit_iters = 1,
    bool threat_aborted_stop_error = true,
    Func<Uptr<Cooldowns>()> fn_createcooldowns = co::bind(&DefaultCapsuleObjectTester::CreateDefaultCooldowns));


  void Execute(LoopExitCause& exit_cause,
               ExceptionState& excp_state);

  void ExecuteExpect(LoopExitCause expected_exit_cause,
                     ExceptionState expected_excp_state = ExceptionState::kNormalExit);

  capsule::RunLoopCounters GetRunLoopCounters() const { return capsule_->GetRunLoopCounters(); }

private:
  // For tests, use minimum cooldowns
  static Uptr<Cooldowns> CreateDefaultCooldowns() {
    return make_unique<capsule::MinimumCooldowns>();
  }

private:
  TestInfo& ti_;
  ThreadModelConfig tm_conf_;
  Uptr<capsule::CapsuleBuilder> builder_;
  Uptr<capsule::Capsule> capsule_;
  bool threat_aborted_stop_error_;
};


}}}

