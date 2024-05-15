#pragma once

#include "co/async/stream_connector.h"
#include "co/async/stream_acceptor.h"
#include "co/async/stream_factory.h"

namespace co {
namespace async {

Uptr<StreamConnector> CreateConnectorForEndpoint(const net::Endpoint& addr);
Uptr<Stream> CreateStreamForEndpoint(io_context& ioc, const net::Endpoint& addr);
Uptr<StreamAcceptor> CreateStreamAcceptorForEndpoint(io_context& ioc, const net::Endpoint& addr);
Uptr<StreamFactory> CreateStreamFactoryForEndpoint(io_context& ioc, const net::Endpoint& addr);

}}



