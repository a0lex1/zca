#pragma once

#include "co/net/cidr_range.h"
#include <string>

namespace co {
namespace net {

// 1.2.3.4-5.6.7.8/24

bool cidr_range_from_string(
    cidr_range&       cr,
    const std::string&   input_string);

}}
