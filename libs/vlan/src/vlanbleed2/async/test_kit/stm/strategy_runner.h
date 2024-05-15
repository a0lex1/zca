#pragma once

#include "vlanbleed2/async/test_kit/stm/strategy.h" //fixnames
#include "co/async/loop_object.h"

//
// Strategy Runner (adapts single Strategy for LoopObject)
//

class StrategyRunner : public co::async::LoopObjectNoreset {
 public:
  using RefTracker = co::RefTracker;

  StrategyRunner(Uptr<Strategy> strat, Uptr<StrategyContext> opaque_context = nullptr)
    : strat_(std::move(strat))
  {
    // relax ctor
    if (opaque_context) {
      strat_->SetOpaqueContext(std::move(opaque_context));
    }
  }

  Strategy& GetStrategy() { return *strat_.get(); }
  void SetStrategyContext(Uptr<StrategyContext> opaque_context) {
    strat_->SetOpaqueContext(std::move(opaque_context));
  }

  // [LoopObject impl]
  void Start(RefTracker rt) override {
    strat_->Start(rt);
  }
  void StopThreadsafe() override {
    // No action taken. User will wait for us to complete naturally.
  }
  void PrepareToStart(Errcode& err) override { }
  void CleanupAbortedStop() override { }

 private:
  Uptr<Strategy> strat_;
};


