#pragma once

#include "./adapter.h"

#include "co/async/service.h"

class VlanService : private co::async::Service {
public:
  virtual ~VlanService() = default;

  using StreamFactory = co::async::StreamFactory;
  using StreamAcceptorFactory = co::async::StreamAcceptorFactory;
  using StreamConnectorFactory = co::async::StreamConnectorFactory;

  VlanService(io_context& ioc, VlanNativeApi& native_api);

  // [StreamIoService impl]
  Uptr<StreamFactory> CreateStreamFactory() override;
  Uptr<StreamAcceptorFactory> CreateStreamAcceptorFactory() override;
  Uptr<StreamConnectorFactory> CreateStreamConnectorFactory() override;

private:
  io_context& ioc_;
  VlanNativeApi& native_api_;
};

