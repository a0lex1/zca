#include "co/base/cmdline/posix_cmdline_wrapper.h"
#include <string.h>

using namespace std;
using namespace co::cmdline;

namespace co {
namespace cmdline {

string cmdline_create(int argc, char* argv[]) {
  if (argc < 1) {
    return "";
  }
  string full;
  for (int i=0; i<argc; i++) {
    if (i > 0) {
      // add space after previous arg
      full += " ";
    }
    if (strchr(argv[i], ' ')) {
      full += "\"";
      full += argv[i];
      full += "\"";
    }
    else {
      full += argv[i];
    }
  }
  return full;
}

string cmdline_create(const posix_cmdline_wrapper<char>& cmdline) {
  return cmdline_create(cmdline.argc_, cmdline.argv_);
}

}}


