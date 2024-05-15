#pragma once

#include <vector>
#include <string>

// C++ adoption of traditional BSD getopts
// class Getopt encapsulates traditional getopt's global vars so it's thread safe

class Getopt {
public:
  int Execute(int argc, char* argv[], const char* optstring);
  int Execute(const std::vector<std::string>& args, const char* optstring);

  struct option {
    const char* name;
    int has_arg;
    int* flag;
    int val;
  };
  static const int no_argument = 1;
  static const int required_argument = 2;
  static const int optional_argument = 3;

  int ExecuteLong(int argc, char* argv[],
    const char* optstring, const option* longopts, int* longindex);

  const char* OptArg() const { return optarg; }
  int OptInd() const { return optind; }

private:
  // RENAME
  const char* optarg;
  int optind{ 1 }, opterr, optopt;
  const char* optcursor{ nullptr };
};

