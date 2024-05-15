#pragma once

#include "co/common.h"
#include <boost/asio/ip/tcp.hpp>

namespace co {
namespace net {

  // ip:port string
void EndpointFromIpPortStr(tcp_endpoint& ep,
  const std::string& ip_port_str, Errcode& err);

tcp_endpoint EndpointFromIpPortStr(
  const std::string& ip_port_str); // throws

/*
  // ip or ip:port string
void EndpointFromIpOrIpPortStr(tcp_endpoint& ep,
  const std::string& ip_or_ip_port_str, Errcode& err);

tcp_endpoint EndpointFromIpOrIpPortStr(
  const std::string& ip_or_ip_port_str); // throws
*/

}}

