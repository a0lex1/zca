#include "co/net/tcp_address_model.h"
#include "co/async/tcp_service.h"
#include "co/async/stream_acceptor_factory.h"
#include "co/async/tcp_stream_factory.h"
#include "co/async/tcp_stream_acceptor.h"
#include "co/async/tcp_stream_connector.h"

using namespace std;

namespace co {
namespace async {

class TcpStreamAcceptorFactory : public StreamAcceptorFactory {
public:
  virtual ~TcpStreamAcceptorFactory() = default;

  TcpStreamAcceptorFactory(io_context& ioc) : ioc_(ioc) {}

  Uptr<StreamAcceptor> CreateStreamAcceptor() override {
    return make_unique<TcpStreamAcceptor>(ioc_);
  }

private:
  io_context& ioc_;
};

class TcpStreamConnectorFactory : public StreamConnectorFactory {
public:
  virtual ~TcpStreamConnectorFactory() = default;

  TcpStreamConnectorFactory(io_context* ioc_ignored) {}

  Uptr<StreamConnector> CreateStreamConnector() override {
    return make_unique<TcpStreamConnector>();
  }
};

// -------------------------------------------------------------

boost::asio::io_context &TcpService::GetIoContext() {
  // Be careful, you must know how you are using ioc_
  DCHECK(ioc_ != nullptr);
  return *ioc_;
}

Uptr<net::AddressModel> TcpService::CreateAddressModel() {
  return make_unique<net::TcpAddressModel>();
}

Uptr<StreamFactory> TcpService::CreateStreamFactory()
{
  DCHECK(ioc_);
  return make_unique<TcpStreamFactory>(*ioc_);
}

Uptr<StreamAcceptorFactory> TcpService::CreateStreamAcceptorFactory()
{
  DCHECK(ioc_);
  return make_unique<TcpStreamAcceptorFactory>(*ioc_);
}

Uptr<StreamConnectorFactory> TcpService::CreateStreamConnectorFactory()
{
  // no DCHECK(ioc_)
  return make_unique<TcpStreamConnectorFactory>(nullptr/*ioc*/);
}

}}

