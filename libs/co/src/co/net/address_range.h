#pragma once

#include "co/common.h"
#include <boost/asio/ip/address.hpp>

namespace co {
namespace net {

class address_range {
public:
  typedef boost::asio::ip::address address;
  typedef boost::asio::ip::address_v4 address_v4;
  typedef boost::asio::ip::address_v6 address_v6;

  address_range() { }
  address_range(address ip_first, address ip_last)
    : ip_first_(ip_first), ip_last_(ip_last)
  {
    DCHECK(ip_first_.is_v4() == ip_last_.is_v4());
  }

  address first_ip() const { return ip_first_; }
  address last_ip() const { return ip_last_; }

  address_range(
      const std::string& ip_first_str,
      const std::string& ip_last_str)
    :
      ip_first_(address::from_string(ip_first_str)),
      ip_last_(address::from_string(ip_last_str))
  {
  }

  bool is_v4() const { return ip_first_.is_v4(); }

  std::string to_string() const {
    return ip_first_.to_string() + "-" + ip_last_.to_string();
  }

private:
  address  ip_first_;
  address  ip_last_;
};

}}
