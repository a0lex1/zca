#include "co/net/address_calc.h"
#include "co/common.h"

#include <boost/asio/ip/address.hpp>
#include <limits>

namespace co {
namespace net {

const uint8_t address_calc::kIPv4AddressBits = 32;
const uint8_t address_calc::kIPv6AddressBits = 128;

using boost::asio::ip::address_v4;
using boost::asio::ip::address_v6;

//////////////////////////

distance address_calc::max_host(bool ipv4, uint8_t network_bits) {
  DCHECK(valid_network_bits(ipv4, network_bits));
  return max_address_as_distance(ipv4).v >> network_bits;
}

distance address_calc::max_net(bool ipv4, uint8_t network_bits) {
  DCHECK(valid_network_bits(ipv4, network_bits));
  return
      max_address_as_distance(ipv4).v >> (address_bits(ipv4) - network_bits);
}

distance address_calc::host_num(const address& addr, uint8_t network_bits)
{
  DCHECK(valid_network_bits(addr.is_v4(), network_bits));
  return
      address_to_distance(addr).v &
      (max_address_as_distance(addr.is_v4()).v >> network_bits);
}

distance address_calc::net_num(const address& addr, uint8_t network_bits)
{
  DCHECK(valid_network_bits(addr.is_v4(), network_bits));
  uint8_t shift_to = address_bits(addr.is_v4()) - network_bits;
  return
      (address_to_distance(addr).v >> shift_to) &
      (max_address_as_distance(addr.is_v4()).v >> shift_to);
}

//////////////////////////

distance address_calc::address_to_distance(const address& addr) {
  if (addr.is_v4()) {
    return co::net::distance::from_dword(addr.to_v4().to_ulong());
  } else {
    DCHECK(!0);
    return distance(0);
  }
}

distance address_calc::max_address_as_distance(bool ipv4) {
  if (ipv4) {
    return co::net::distance::from_dword(0xffffffff);
  } else {
    return max_uint128_t();
  }
}

address_calc::address  address_calc::make_address(
    bool ipv4,  uint8_t network_bits,  const distance& NetNum,
    const distance& HostNum)
{
  DCHECK(valid_network_bits(ipv4, network_bits));
  if (ipv4) {
#ifndef _NDEBUG
    if (NetNum.v > max_value(network_bits).v) {
      DCHECK(!"NetNum > max value");
      return address_v4();
    }
    if (!HostNum.v > max_value(kIPv4AddressBits - network_bits).v) {
      DCHECK(!"HostNum > max value");
      return address_v4();
    }
#endif
    return address_v4(
          NetNum.to_dword() << (kIPv4AddressBits - network_bits)
          |
          HostNum.to_dword()
          );
  }
  else
  {
    DCHECK(!"IPv6 not supported");
    return address_v6();
  }
}

uint8_t address_calc::address_bits(bool ipv4) {
  if (ipv4) {
    return kIPv4AddressBits;
  } else {
    return kIPv6AddressBits;
  }
}

bool address_calc::valid_net_num(bool ipv4, uint8_t network_bits,
                 const distance& NetNum)
{
  return NetNum.v <= max_net(ipv4, network_bits).v;
}

bool address_calc::valid_host_num(bool ipv4, uint8_t network_bits,
                  const distance& HostNum)
{
  return HostNum.v <= max_host(ipv4, network_bits).v;
}

bool address_calc::valid_network_bits(bool ipv4, uint8_t network_bits) {
  if (0 == network_bits) {
    return false;
  }
  if (ipv4) {
    return network_bits < kIPv4AddressBits;
  }
  else {
    return network_bits < kIPv6AddressBits;
  }
}

distance address_calc::max_value(uint8_t num_bits) {
  DCHECK(num_bits != 0);
  return max_uint128_t() >> (128 - num_bits);
}

distance::uint128_t address_calc::max_uint128_t() {
  return std::numeric_limits<distance::uint128_t>::max();
}

}}
