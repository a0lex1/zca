#pragma once

#include "co/common.h"
#include "co/net/endpoint.h"

#include <boost/asio/buffer.hpp>

namespace co {
namespace async {

/*
Only I/O
*/
class StreamIo {
public:
  virtual ~StreamIo() = default;

  StreamIo(io_context& ioc) : ioc_(&ioc) {}

  boost::asio::io_context& GetIoContext() {
    DCHECK(ioc_);
    return *ioc_;
  }

  virtual void AsyncReadSome(
    boost::asio::mutable_buffers_1 buf, HandlerWithErrcodeSize handler) = 0;

  virtual void AsyncWriteSome(
    boost::asio::const_buffers_1 buf, HandlerWithErrcodeSize handler) = 0;

private:
  io_context* ioc_{nullptr};
};

/*
Stream with i/o and close/shutdown operations
*/
class Stream : public StreamIo {
public:
  virtual ~Stream() { }

  using StreamIo::StreamIo;

  // (StreamIo methods were here)

  virtual void Shutdown(Errcode& err) = 0;

  virtual void Cancel(Errcode& err) = 0;

  virtual void Close() = 0;

  virtual bool IsOpen() const = 0;

  virtual void GetLocalAddress(co::net::Endpoint&, Errcode&) = 0;
  virtual void GetRemoteAddress(co::net::Endpoint&, Errcode&) = 0;
};

}
}
