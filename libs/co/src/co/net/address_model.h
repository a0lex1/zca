#pragma once

#include "co/net/endpoint.h"

#include <system_error>

namespace co {
namespace net {

class AddressModel {
public:
  virtual ~AddressModel() = default;
  
  // non-const. AddressModel is opaque.
  // Use GetDefaultEndpoint() for FromString()
  virtual Endpoint GetDefaultEndpoint() = 0;
  virtual net::Endpoint GetLoopbackEndpoint() = 0;
  virtual net::Endpoint GetThisHostEndpoint() = 0;
  
/*
  virtual void EndpointFromString(const std::string& ep_str,
                                  net::Endpoint& ep,
                                  std::error_condition& errcond) = 0;
                                  
  virtual void EndpointToString(std::string& ep_str) = 0;*/
};


}}


