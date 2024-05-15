#pragma once

#include "co/base/strings.h"

namespace co {

class Location {
public:
  Location(bool ignored) {}
  Location() :line_(-1) {}
  Location(std::string file, int line, std::string func) : file_(file), line_(line), func_(func) {

  }
  const std::string& GetFile() const { return file_; }
  int GetLine() const { return line_; }
  const std::string& GetFunc() const { return func_; }

  std::string ToString() const {
    return "@" + file_ + ":" + string_from_int(line_) + " (func " + func_ + ")";
  }

private:
  std::string file_;
  int line_;
  std::string func_;
};

#ifndef CO_DISABLE_LOCATION
#define CUR_LOC() ::co::Location(__FILE__, __LINE__, __FUNCTION__)
#else
#define CUR_LOC() ::co::Location(false/*ignored*/)
#endif


}

