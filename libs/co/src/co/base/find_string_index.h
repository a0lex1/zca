#pragma once

#include "co/common.h"

namespace co {
  static size_t FindStringIndex(const StringVector& sv, const std::string& str) {
    for (size_t i = 0; i < sv.size(); i++) {
      if (sv[i] == str) {
        return i;
      }
    }
    return -1;
  }
}
