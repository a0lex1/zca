#pragma once

#include "co/net/distance.h"

#include <stdint.h>

namespace co {
namespace net {

class address_calc {
public:
  typedef boost::asio::ip::address address;

  static distance max_host(bool ipv4, uint8_t network_bits);
  static distance max_net(bool ipv4, uint8_t network_bits);

  static distance host_num(const address& addr, uint8_t network_bits);
  static distance net_num(const address& addr, uint8_t network_bits);

  static distance address_to_distance(const address& addr);
  static distance max_address_as_distance(bool ipv4);

  static address make_address(bool ipv4, uint8_t network_bits,
                const distance& NetNum, const distance& HostNum);

  static uint8_t address_bits(bool ipv4);

  static bool valid_host_num(bool ipv4, uint8_t network_bits,
                 const distance& HostNum);

  static bool valid_net_num(bool ipv4, uint8_t network_bits,
                const distance& NetNum);

  static bool valid_network_bits(bool ipv4, uint8_t network_bits);

  static distance max_value(uint8_t num_bits);

private:
  static const uint8_t kIPv4AddressBits;
  static const uint8_t kIPv6AddressBits;

  static distance::uint128_t max_uint128_t();
};

}}
