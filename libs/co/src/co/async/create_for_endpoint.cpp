#include "co/async/create_for_endpoint.h"
#include "co/async/tcp_stream_connector.h"
#include "co/async/tcp_stream_acceptor.h"
#include "co/async/tcp_stream_factory.h"

using namespace std;

using co::net::detail::CO_ENDPOINT_ID_TCP;

namespace co {
namespace async {

// TODO: this will probably be CreateServiceForEndpoint, would require some Registrator

Uptr<StreamConnector> CreateConnectorForEndpoint(const net::Endpoint& addr) {
  switch (addr.GetType()) {
  case CO_ENDPOINT_ID_TCP: return make_unique<TcpStreamConnector>();
  }
  NOTREACHED();
  return nullptr;
}

Uptr<Stream> CreateStreamForEndpoint(io_context& ioc,
                                     const net::Endpoint& addr) {
  switch (addr.GetType()) {
  case CO_ENDPOINT_ID_TCP: return make_unique<TcpStream>(ioc);
  }
  return nullptr;
}

Uptr<StreamAcceptor> CreateStreamAcceptorForEndpoint(io_context& ioc,
                                                     const net::Endpoint& addr) {
  switch (addr.GetType()) {
  case CO_ENDPOINT_ID_TCP: return make_unique<TcpStreamAcceptor>(ioc);
  }
  return nullptr;
}

Uptr<StreamFactory> CreateStreamFactoryForEndpoint(io_context& ioc,
                                                   const net::Endpoint& addr) {
  switch (addr.GetType()) {
  case CO_ENDPOINT_ID_TCP: return make_unique<TcpStreamFactory>(ioc);
  }
  NOTREACHED();
  return nullptr;
}

}}


