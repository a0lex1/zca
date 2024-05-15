#pragma once

#include "./acceptor_impl.h"
#include "./connector_impl.h"
#include "./stream_impl.h"

#include "co/async/stream_acceptor_factory.h"
#include "co/async/stream_connector_factory.h"
#include "co/async/stream_factory.h"

class StreamFactoryImpl : public co::async::StreamFactory {
public:
  virtual ~StreamFactoryImpl() = default;

  using Stream = co::async::Stream;

  StreamFactoryImpl(io_context& ioc_adapter): StreamFactory(ioc_adapter)
  {
  }

  Uptr<Stream> CreateStream() override {
    return make_unique<StreamImpl>(GetIoContext());
  }
};

class StreamAcceptorFactoryImpl : public co::async::StreamAcceptorFactory {
public:
  virtual ~StreamAcceptorFactoryImpl() = default;

  using StreamAcceptor = co::async::StreamAcceptor;

  StreamAcceptorFactoryImpl(io_context& ioc,
                         VlanNativeApi& native_api)
    : ioc_(ioc), native_api_(native_api)
  {
  }

  Uptr<StreamAcceptor> CreateStreamAcceptor() override {
    return make_unique<StreamAcceptorImpl>(ioc_, native_api_);
  }
private:
  io_context& ioc_;
  VlanNativeApi& native_api_;
};

class StreamConnectorFactoryImpl : public co::async::StreamConnectorFactory {
public:
  virtual ~StreamConnectorFactoryImpl() = default;

  using StreamConnector = co::async::StreamConnector;

  StreamConnectorFactoryImpl(VlanNativeApi& native_api)
    : native_api_(native_api)
  {

  }

  Uptr<StreamConnector> CreateStreamConnector() override {
    return std::make_unique<StreamConnectorImpl>(native_api_);
  }
private:
  VlanNativeApi& native_api_;
};
