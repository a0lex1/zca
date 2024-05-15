#pragma once

#include "co/net/cidr_range.h"

#include <stdint.h>
#include <vector>
#include <string>

// 1.2.3.4-5.6.7.8/13;100.0.0.1-101.2.2.5/24

namespace co {
namespace net {

bool cidr_ranges_from_string(
    const std::string&       input_string,
    std::vector<cidr_range>&   cidr_ranges_to_append);

}}
