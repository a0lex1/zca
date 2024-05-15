#pragma once

#include <boost/asio/ip/tcp.hpp>

namespace co {
namespace net {

bool endpoint_from_sockaddr(
    boost::asio::ip::tcp::endpoint& ep,
    const sockaddr* name,
    int namelen);

}}
