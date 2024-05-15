#pragma once

#include <boost/asio/ip/address.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <assert.h>
#include <sstream>
#include <stdint.h>

namespace co {
namespace net {

struct distance {
  typedef boost::multiprecision::uint128_t uint128_t;

  uint128_t v;

  distance(uint128_t V = 0): v(V) { }

  static  distance from_dword(uint32_t V) {
    distance ret;
    ret.v = V;
    return ret;
  }

  bool can_be_dword() const { return v <= 0xffffffff; }
  uint32_t to_dword() const { return v.convert_to<uint32_t>(); }
};


}}

// ---

co::net::distance operator-(
    const boost::asio::ip::address& sub_from,
    const boost::asio::ip::address& sub_what);

boost::asio::ip::address operator+(
    const boost::asio::ip::address& addr,
    const co::net::distance& d);

boost::asio::ip::address operator-(
    const boost::asio::ip::address& addr,
    const co::net::distance& d);

co::net::distance operator+(
    const co::net::distance& d, co::net::distance::uint128_t a);

co::net::distance operator+(
    const co::net::distance& d, const co::net::distance& a);

co::net::distance operator-(
    const co::net::distance& d, co::net::distance::uint128_t a);

co::net::distance operator-(
    const co::net::distance& d, const co::net::distance& a);
