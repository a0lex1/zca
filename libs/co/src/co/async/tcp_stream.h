#pragma once

#include "co/async/stream.h"
#include "co/common.h"
#include <boost/asio/ip/tcp.hpp>

namespace co {
namespace async {

class TcpStream : public Stream {
public:
  TcpStream(io_context& ioc) : Stream(ioc), tcp_sock_(ioc) { }

  virtual ~TcpStream() { }

  tcp_socket& GetSocket();

  // stream interface
  void AsyncReadSome(
    boost::asio::mutable_buffers_1 buf, HandlerWithErrcodeSize handler) override;

  void AsyncWriteSome(
    boost::asio::const_buffers_1 buf, HandlerWithErrcodeSize handler) override;

  void Shutdown(Errcode& err) override;

  void Cancel(Errcode& err) override;

  void Close() override;

  bool IsOpen() const override;

  void GetLocalAddress(co::net::Endpoint&, Errcode&) override;
  void GetRemoteAddress(co::net::Endpoint&, Errcode&) override;

private:
  // internal

private:
  tcp_socket tcp_sock_;
};

}
}
