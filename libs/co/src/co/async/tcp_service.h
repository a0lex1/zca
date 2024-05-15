#pragma once

#include "co/async/service.h"

namespace co {
namespace async {

class TcpService: public Service {
public:
  virtual ~TcpService() = default;

  TcpService() { /* must use SetIoContext() */ }
  TcpService(io_context& ioc) : ioc_(&ioc) {}

  // [Service impl]

  // If was not set in CTOR
  void SetIoContext(io_context& ioc) override { ioc_ = &ioc; }
  io_context& GetIoContext() override;

  Uptr<co::net::AddressModel> CreateAddressModel() override;
  Uptr<StreamFactory> CreateStreamFactory() override;
  Uptr<StreamAcceptorFactory> CreateStreamAcceptorFactory() override;
  Uptr<StreamConnectorFactory> CreateStreamConnectorFactory() override;

private:
  io_context* ioc_{ nullptr };
};

}}
