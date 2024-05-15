#pragma once

#include "co/base/cmdline/keyed_cmdline.h"
#include <vector>
#include <map>
#include <string>

#include <boost/noncopyable.hpp>

namespace co {

struct TestInfo {
  std::map<std::string, std::string> opts_dict;
};

struct TestTableEntry {
  void(* lpfn_test_func)(TestInfo& info);
  const char* func_name;
};
using TestTable = std::vector<TestTableEntry>;

#define ADD_TEST(func) { func, #func }

// used in test_co_xlog()
void TemporaryUninitsyslog();
void TemporaryInitsyslogAgain();

int RunTests(
  cmdline::KeyedCmdLine<char>& cmd_line,
  const std::vector<std::pair<std::string, co::TestTable>>& tables,
  bool need_only_one = false);

}


