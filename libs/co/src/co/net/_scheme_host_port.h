// TODO

#pragma once

#include <string>
#include <stdint.h>

namespace co {
namespace net {

class SchemeHostPort {
public:
  SchemeHostPort(std::string uri) { }

  bool Valid() const { return true; }
  std::string scheme() const { return ""; }
  std::string host() const { return ""; }
  uint16_t port() const { return 0; }
};

}}
