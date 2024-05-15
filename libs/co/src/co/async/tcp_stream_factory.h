#pragma once

#include "co/async/stream_factory.h"
#include "co/async/tcp_stream.h"
#include "co/common.h"

namespace co {
namespace async {

class TcpStreamFactory: public StreamFactory {
public:
  using StreamFactory::StreamFactory;

  virtual Uptr<Stream> CreateStream() override {
    return std::make_unique<TcpStream>(GetIoContext());
  }

  virtual ~TcpStreamFactory() = default;
};

}}
