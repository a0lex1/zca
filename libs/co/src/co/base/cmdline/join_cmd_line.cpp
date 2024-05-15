#include "co/base/cmdline/join_cmd_line.h"

using namespace std;

namespace co {
namespace cmdline {

string JoinCmdLine(int argc, char* argv[]) {
  // If space found, wrap with quotes
  string final_cmdline;
  for (int i=0; i<argc; i++) {
    if (strchr(argv[i], ' ')) {
      final_cmdline += string("\"") + argv[i] + string("\"");
    } else {
      final_cmdline += argv[i];
    }
    if (i < argc - 1) {
      final_cmdline += " ";
    }
  }
  return final_cmdline;
}

string JoinCmdLine(const StringVector& args, size_t start_from) {
  DCHECK(start_from < args.size());
  vector<char*> argv;
  for (size_t i = start_from; i < args.size(); i++) {
    argv.push_back(const_cast<char*>(args[i].c_str()));
  }
  return JoinCmdLine(argv.size(), &argv[0]);
}

}}

