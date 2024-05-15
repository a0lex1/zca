#pragma once

#include "co/async/stream.h"
#include "co/common.h"

namespace co {
namespace async {

class StreamFactory {
public:
  virtual ~StreamFactory() = default;

  StreamFactory(io_context& ioc) : ioc_(ioc) {}

  io_context& GetIoContext() { return ioc_; }

  virtual Uptr<Stream> CreateStream() = 0;

private:
  io_context& ioc_;
};

}}


