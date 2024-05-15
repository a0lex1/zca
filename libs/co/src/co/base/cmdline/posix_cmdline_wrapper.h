#pragma once

#include <stdlib.h>
#include <string>

// Command line utilities.

namespace co {
namespace cmdline {

template <typename CharT>
class posix_cmdline_wrapper {
private:
  void freeIfUsed() {
    if (argc_ != 0) {
      DCHECK(argv_ != nullptr);
      for (int i=0; i<argc_; ++i) {
        free(argv_[i]);
        argv_[i] = nullptr;
      }
      free(argv_);
      argv_ = nullptr;
    } else {
      DCHECK(argv_ == nullptr);
    }
  }

  void copyFrom(posix_cmdline_wrapper& r) {
    freeIfUsed();
    argc_ = r.argc_;
    argv_ = malloc(argc_ * sizeof(CharT*));
    for (int i=0; i<argc_; ++i) {
      switch (sizeof(CharT)) {
      default:
      case sizeof(char):
        argv_[i] = strdup(r.argv_[i]);
        break;
      case sizeof(wchar_t):
        argv_[i] = wcscup(r.argv_[i]);
        break;
      }
      // low-mem handling: free already allocated if error
    }
  }

public:
  // Fields
  int argc_;
  CharT** argv_;

  // Methods
  ~posix_cmdline_wrapper() { freeIfUsed(); }

  posix_cmdline_wrapper(): argc_(0), argv_(nullptr) { }
  posix_cmdline_wrapper(int argc, CharT* argv[]): argc_(argc), argv_(argv) { }

  posix_cmdline_wrapper(const posix_cmdline_wrapper& r) {
    copyFrom(r);
  }
};

// --------------------------------------------------

std::string cmdline_create(int argc, char* argv[]);
std::string cmdline_create(const posix_cmdline_wrapper<char>& cmdline);


}}

