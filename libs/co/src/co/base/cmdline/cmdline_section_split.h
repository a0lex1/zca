#pragma once

#include "co/common.h"

namespace co {
namespace cmdline {

// returns false if unclosed quote/dquote
// splitter can be something like " -- " or " | "
bool CmdlineSectionSplit(const std::string& raw_cmdline, StringVector& sections,
  const std::string& splitter);

}}


