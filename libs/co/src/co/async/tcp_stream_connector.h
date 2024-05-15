#pragma once

#include "co/async/tcp_stream.h"
#include "co/async/stream_connector.h"
#include "co/xlog/define_logger_sink.h"

namespace co {
namespace async {

DECLARE_XLOGGER_SINK("connector", gCoAsyncStreamConnectorLogSink);

class TcpStreamConnector: public StreamConnector {
public:
  virtual ~TcpStreamConnector() { }

  using Endpoint = net::Endpoint;
  
  void AsyncConnect(Endpoint addr, Stream& stm, HandlerWithErrcode handler) override;
};

}}

