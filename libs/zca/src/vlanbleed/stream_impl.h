#pragma once

#include "./native_api.h"

#include "co/async/stream.h"

class StreamImpl : public co::async::Stream {
public:
  ~StreamImpl() = default;

  StreamImpl(io_context& ioc_adap) : Stream(ioc_adap) {}

  void SetNativeApi(VlanNativeApi& native_api) {
    native_api_ = &native_api;
  }

  vlhandle_t& Handle() {
    return handle_;
  }

  // [StreamIo impl]
  void AsyncReadSome(
    boost::asio::mutable_buffers_1 buf, HandlerWithErrcodeSize handler) override
  {
  }
  void AsyncWriteSome(
    boost::asio::const_buffers_1 buf, HandlerWithErrcodeSize handler) override
  {
  }

  // [Stream impl]
  void Shutdown(Errcode& err) override
  {
  }

  void Cancel(Errcode& err) override
  {
  }

  void Close() override
  {
  }

  bool IsOpen() const override
  {
    return false;
  }

private:
  vlhandle_t handle_{ 0 };
  VlanNativeApi* native_api_{ nullptr };
};

