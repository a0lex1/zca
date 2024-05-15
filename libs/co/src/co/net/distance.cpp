#include "co/net/distance.h"
#include "co/common.h"

// Global namespace

co::net::distance operator-(
    const boost::asio::ip::address& sub_from,
    const boost::asio::ip::address& sub_what)
{
  DCHECK(sub_from.is_v4() == sub_what.is_v4());
  if (sub_from.is_v4()) {
    return
        co::net::distance::from_dword(sub_from.to_v4().to_ulong() - sub_what.to_v4().to_ulong());
  }
  else {
    DCHECK(!"ipv6 not implemented");
    return co::net::distance();
  }
}

boost::asio::ip::address operator+(
    const boost::asio::ip::address& addr,
    const co::net::distance& d)
{
  if (addr.is_v4()) {
    DCHECK(d.can_be_dword());
    return boost::asio::ip::address_v4(
          addr.to_v4().to_ulong() + d.to_dword());
  }
  else {
    DCHECK(!"ipv6 not implemented");
    return boost::asio::ip::address();
  }
}

boost::asio::ip::address operator-(
    const boost::asio::ip::address& addr,
    const co::net::distance& d)
{
  if (addr.is_v4()) {
    DCHECK(d.can_be_dword());
    return boost::asio::ip::address_v4(
          addr.to_v4().to_ulong() - d.to_dword());
  }
  else {
    DCHECK(!"ipv6 not implemented");
    return boost::asio::ip::address();
  }
}

co::net::distance operator+(
    const co::net::distance& d, co::net::distance::uint128_t a)
{
  return d.v + a;
}

co::net::distance operator+(
    const co::net::distance& d, const co::net::distance& a)
{
  return d.v + a.v;
}
