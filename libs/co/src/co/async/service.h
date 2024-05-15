#pragma once

#include "co/async/stream_factory.h"
#include "co/async/stream_acceptor_factory.h"
#include "co/async/stream_connector_factory.h"

#include "co/net/address_model.h"

namespace co {
namespace async {

class Service {
public:
  virtual ~Service() = default;

  virtual void SetIoContext(io_context& ioc) = 0;

  virtual io_context& GetIoContext() = 0;

  virtual Uptr<net::AddressModel> CreateAddressModel() = 0;

  virtual Uptr<StreamFactory> CreateStreamFactory() = 0;

  virtual Uptr<StreamAcceptorFactory> CreateStreamAcceptorFactory() = 0;

  virtual Uptr<StreamConnectorFactory> CreateStreamConnectorFactory() = 0;
};

}}

