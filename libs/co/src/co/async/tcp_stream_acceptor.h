#pragma once

#include "co/async/stream_acceptor.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

// #WorkFeatures
#define CO_ACPT_TCP_REUSEADDR

namespace co {
namespace async {

class TcpStreamAcceptor: public StreamAcceptor {
public:
  TcpStreamAcceptor(io_context& ioc);

  bool IsOpen() override;
  void Open(Errcode&) override;
  void Bind(Endpoint, Errcode&) override;
  void StartListening(Errcode&) override;
  void GetLocalAddress(Endpoint&, Errcode&) override;
  void GetLocalAddressToConnect(Endpoint&, Errcode&) override;
  void Close() override;
  void CancelAccept(Errcode&) override;
  void AsyncAccept(Stream&, HandlerWithErrcode handler) override;

  using StreamAcceptor::Open;
  using StreamAcceptor::Bind;
  using StreamAcceptor::StartListening;

  virtual ~TcpStreamAcceptor() { }

private:
  tcp_acceptor acpt_;
};


}
}
