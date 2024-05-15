#include "co/net/asio_endpoint_from_string.h"
#include "co/net/parse_host_port.h"

using namespace std;

namespace co {
namespace net {

// returns boost::asio error
void EndpointFromIpPortStr(tcp_endpoint& ep,
  const std::string& ip_port_str, Errcode& err)
{
  string ip;
  u_short port;
  if (!parse_host_port(ip_port_str, ip, port)) {
    err = boost::asio::error::invalid_argument;
    return;
  }
  ip_address ipaddr = ip_address::from_string(ip, err);
  if (err) {
    return;
  }
  // Success
  ep = tcp_endpoint(ipaddr, port);
  err = NoError();
}

tcp_endpoint EndpointFromIpPortStr(
  const std::string& ip_port_str)
{
  Errcode err;
  tcp_endpoint ep;
  EndpointFromIpPortStr(ep, ip_port_str, err);
  if (err) {
    BOOST_THROW_EXCEPTION(boost::system::system_error(err));
  }
  return ep;
}

}}

