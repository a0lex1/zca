#pragma once

#include "co/common.h"

namespace co {
namespace cmdline {

static std::vector<char*> MakeRawArgv(const StringVector& args) {
  std::vector<char*> result;
  for (const auto& arg : args) {
    result.push_back(const_cast<char*>(arg.c_str()));
  }
  return result;
}

}}

