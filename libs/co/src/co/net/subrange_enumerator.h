#pragma once

#include "co/net/distance.h"
#include "co/net/address_range.h"
#include "co/common.h"

#include <algorithm>

namespace co {
namespace net {

class subrange_enumerator {
public:
  typedef co::net::address_range address_range;
  typedef boost::asio::ip::address address;

  subrange_enumerator() { }
  subrange_enumerator(
  const address_range& full_range, const distance& piece_len)
  :
  full_range_(full_range),
  piece_len_(piece_len), cur_addr_(full_range.first_ip())
  {
  assert(piece_len_.v > 0);
  //assert(full_range_.is_v4() == piece_len_.can_be_dword() == cur_addr_.is_v4());

  distance total(full_range_.last_ip() - full_range_.first_ip());
  ++total.v;
  if (piece_len_.v > total.v) {
    piece_len_ = total;
  }
  }

  bool end() const {
  return cur_addr_ == full_range_.last_ip() + distance(1);
  }

  address_range current_subrange() const {
  return address_range(cur_addr_, cur_addr_ + piece_len_ - distance(1));
  }

  void next() {
  cur_addr_ = cur_addr_ + piece_len_;

  distance left(full_range_.last_ip() - cur_addr_);
  ++left.v;
  if (piece_len_.v > left.v) {
    piece_len_ = left;
  }
  }

private:
  address_range   full_range_;
  distance      piece_len_;
  address       cur_addr_;
};

}}
