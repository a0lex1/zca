#pragma once

#include "co/base/tests.h"

#include <map>
#include <string>

int test_main(int argc, char* argv[],
  const std::vector<std::pair<std::string, co::TestTable>>& tables,
  bool need_only_one = false);

