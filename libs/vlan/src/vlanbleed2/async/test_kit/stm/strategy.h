#pragma once

#include "vlanbleed2/async/test_kit/stm/strategy_context.h"

#include "co/async/startable_stopable.h"
#include "co/base/opaque_object.h"

// Strategy (encapsulates only how to work with opaque context)

class Strategy : public co::async::Startable {
 public:
  virtual ~Strategy() = default;

  Strategy(Uptr<StrategyContext> opaque_sock = nullptr)
    : opaque_context_(std::move(opaque_sock))
  {
  }

  void SetOpaqueContext(Uptr<StrategyContext> opaque_sock) {
    opaque_context_ = std::move(opaque_context_);
  }

  // [Startable::Start]

 protected:
  StrategyContext& GetStrategyContext() { return *opaque_context_; }
  //io_context& GetIoctxForCallback() { return ioc_for_cbk_; }

 private:
  Uptr<StrategyContext> opaque_context_;
};


