#pragma once

#include "vlanbleed2/async/test_kit/stm/strategy_context.h"

#include "co/async/stream.h"

// ???????//
// Implements Opaque Socket through encapsulated Stream's interface

class OpaqueContextStream : public StrategyContext {
 public:
  virtual ~OpaqueContextStream() = default;

  using Stream = co::async::Stream;

  OpaqueContextStream(Uptr<Stream> stm)
    : stm_(std::move(stm))
  {
  }
 private:
  Uptr<Stream> stm_;
};
