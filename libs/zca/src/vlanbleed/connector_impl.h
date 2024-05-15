#pragma once

#include "./stream_impl.h"
#include "co/async/stream_connector.h"

class StreamConnectorImpl : public co::async::StreamConnector {
public:
  virtual ~StreamConnectorImpl() = default;

  using Endpoint = co::net::Endpoint;
  using Stream = co::async::Stream;

  StreamConnectorImpl(VlanNativeApi& native_api)
    : native_api_(native_api)
  {

  }

  void AsyncConnect(Endpoint addr, Stream& stm, HandlerWithErrcode uhandler) override {
    const auto& vladdr(static_cast<VlanEndpoint&>(addr));
    auto& vlstm(static_cast<StreamImpl&>(stm));
    connecting_ = true;
    native_api_.VnConnect(vlstm.Handle(), addr,
                          [&, uhandler, addr](const VlanError& vle) {
                            connecting_ = false;
                            if (!vle) {
                              // vlstm.Handle() already has the handle
                              vlstm.SetNativeApi(native_api_);
                            }
                            Errcode e = vle.ToErrcodeDefault();
                            uhandler(e);
                          },
                          stm.GetIoContext());
  }

private:
  VlanNativeApi& native_api_;
  bool connecting_{ false };
};

