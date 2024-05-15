#pragma once

#include "co/base/convert_value.h"

#include "co/net/address_model.h"

#include <system_error>

namespace co {
namespace net {

class TcpAddressModel: public net::AddressModel {
public:
  virtual ~TcpAddressModel() = default;

  using Endpoint = net::Endpoint;

  // Example: GetDefaultEndpoint().FromString("abc:e", err)
  // Example: GetDefaultEndpoint().FromString("abc:e", err).ToString()

  Endpoint GetDefaultEndpoint() override {
    return GetThisHostEndpoint();
  }
  Endpoint GetLoopbackEndpoint() override {
    return TcpEndpoint("127.0.0.1", 0);
  }
  Endpoint GetThisHostEndpoint() override {
    return TcpEndpoint("0.0.0.0", 0);
  }
};

}}

