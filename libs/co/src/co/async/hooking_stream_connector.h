#pragma once

#include "co/async/stream_connector.h"

namespace co {
namespace async {

// A connector that has an underlying connector for connecting hooked streams
class HookingStreamConnector: public StreamConnector {
public:
  virtual ~HookingStreamConnector() = default;

  HookingStreamConnector(Uptr<StreamConnector> underlying_connector)
    :
    underlying_connector_uptr_(std::move(underlying_connector)),
    underlying_connector_ptr_(underlying_connector_uptr_.get())
  {
  }

  HookingStreamConnector(StreamConnector& underlying_connector)
    :
    underlying_connector_ptr_(&underlying_connector)
  {
  }

  void AsyncConnect(Endpoint addr, Stream& stm, HandlerWithErrcode handler) override {
    HookingStream& hooking_stm = static_cast<HookingStream&>(stm);
    GetUnderlyingConnector().AsyncConnect(addr, hooking_stm.GetUnderlyingStream(), handler);
  }

  StreamConnector& GetUnderlyingConnector() { return *underlying_connector_ptr_; }
  const StreamConnector& GetUnderlyingConnector() const { return *underlying_connector_ptr_; }

private:
  Uptr<StreamConnector> underlying_connector_uptr_;
  StreamConnector* underlying_connector_ptr_;
};


}}

