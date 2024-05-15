#pragma once

#include "co/base/strings.h"

#include <string>
#include <vector>

namespace co {
namespace cmdline {

struct IntermedCmdRepr {
  std::vector<std::string> parts;

  std::string ProgramName() const { return parts[0]; }

  IntermedCmdRepr() {}
  IntermedCmdRepr(const std::vector<std::string>& _parts) : parts(_parts) {}

  /*
  * |parts| always have element0 that is empty even when constructed with empty |raw_cmd|
  * This means it's safe to do:
  *   IntermedCmdRepr(any_string_even_empty_string).parts[0]
  */
  IntermedCmdRepr(const std::string& raw_cmd) {
    co::string_split(raw_cmd, " ", parts);
  }

  std::string Join() const {
    std::string buf;
    for (size_t i = 0; i < parts.size(); i++) {
      if (i > 0) {
        buf += " ";
      }
      buf += parts[i];
    }
    return buf;
  }

};

}}

