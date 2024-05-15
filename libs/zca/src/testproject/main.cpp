#include "co/base/cmdline/cmdline_section_split.h"
#include "co/base/cmdline/parse_cmdline_to_argv.h"

#include <cstdlib>
#include <iostream>
#include <exception>
#include <cstdio>
#include <vector>

using namespace std;

int main(int argc, char* argv[])
{
  StringVector secs;
  //co::cmdline::CmdlineSectionSplit("abc' -- 'hello", secs, " -- ");

  co::cmdline::ParseCmdlineToArgv("abc 'aaa'", secs);
  return 0;
}

