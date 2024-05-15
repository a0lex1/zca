#pragma once

#include "co/net/endpoint.h"
#include "co/async/stream.h"
#include "co/common.h"

namespace co {
namespace async {

class StreamConnector {
public:
  virtual ~StreamConnector() { }

  using Endpoint = net::Endpoint;
  
  virtual void AsyncConnect(Endpoint addr, Stream& stm, HandlerWithErrcode handler) = 0;
};

}}

