#pragma once

#include "co/base/cmdline/parse_cmdline_to_argv.h"
#include "co/base/cmdline/make_raw_argv.h"

namespace co {
namespace cmdline {

class ParsedCommandLine {
public:
  ParsedCommandLine() {}

  ParsedCommandLine(const char* raw_cmdline) {
    parser_ret_ = Parse(raw_cmdline);
  }

  int Parse(const char* raw_cmdline) {
    if (!ParseCmdlineToArgv(raw_cmdline, args_)) {
      return -1;
    }
    argv_ = MakeRawArgv(args_);
    return 0;
  }
  int GetParserRet() const {
    return parser_ret_;
  }
  int GetArgc() const {
    return static_cast<int>(argv_.size());
  }
  char** GetArgv() const {
    return const_cast<char**>(&argv_[0]);
  }
private:
  int parser_ret_{ 0 };
  StringVector args_;
  std::vector<char*> argv_;
};

}}


