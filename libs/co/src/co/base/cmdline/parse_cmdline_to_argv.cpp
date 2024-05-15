#include "co/base/cmdline/parse_cmdline_to_argv.h"

using namespace std;

namespace co {
namespace cmdline {

static bool isprn(char c) {
  return c >= 0x20 && c <= 0x7e;
}

static bool iswhitespace(char c) {
  return c == ' ' || c == '\t';
}

bool ParseCmdlineToArgv(const char* cmdline, vector<string>& argv) {
  argv.clear();

  static const size_t kNotYetOpened = -1;
  string cur_arg;
  const char* c = cmdline;
  size_t quote_at = kNotYetOpened;
  char quote_char;
  bool cleared = false;

  for (size_t i = 0; cmdline[i] != '\0'; i++) {
    if (!isprn(c[i])) { //!!!!!!!!!!!!!!!!!!! unnecessary ?
      // bad character found
      return false;
    }
    if (quote_at == kNotYetOpened) {
      if (iswhitespace(c[i])) {
        if (!cur_arg.empty()) {
          // dump arg
          argv.push_back(cur_arg);
          cur_arg.clear();
        }
      }
      while (iswhitespace(c[i])) {
        ++i; // \0 may be reached
      }
    }
    if (c[i] == '\0') {
      // \0 may be reached after recent while consuming whitespaces
      break;
    }
    if (c[i] == '"' || c[i] == '\'') {
      if (quote_at == kNotYetOpened) {
        // begin quote, consume next char
        quote_at = i;
        quote_char = c[i];
        continue;
      }
      else {
        if (c[i] == quote_char) {
          // end of quote, consume next char
          quote_at = kNotYetOpened;
          // if this was an empty arg ('' or ""), add it
          if (cur_arg.empty()) {
            argv.emplace_back("");
          }
          continue;
        }
        else {
          // other quote char inside quote (like "hello '123' world")
          // continue adding chars to |cur_arg|
        }
      }
    }
    cur_arg += c[i];
  }
  if (quote_at != kNotYetOpened) {
    // End of string while parsing doublequoted contents
    return false;
  }
  if (!cur_arg.empty()) {
    argv.push_back(cur_arg);
  }
  return true;
}


}}

