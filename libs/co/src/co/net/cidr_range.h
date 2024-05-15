#pragma once

#include "co/net/address_range.h"

#include <boost/lexical_cast.hpp>

#include <stdint.h>

namespace co {
namespace net {

struct cidr_range {
  address_range  range;
  uint8_t      network_bits;

  bool valid() const;

  std::string to_string() const;
};

}}
