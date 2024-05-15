
#pragma once

#include <string>

namespace co {
namespace net {

bool parse_scheme_address(const std::string& scheme_address,
                          std::string& scheme,
                          std::string& address);

}}
