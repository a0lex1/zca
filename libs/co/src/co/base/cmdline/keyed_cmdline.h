#pragma once

#include "co/base/cmdline/parsed_command_line.h"

#include <map>
#include <vector>
#include <string>
#include <ostream>
#include <sstream>

namespace co {
namespace cmdline {

template <typename C>
class KeyedCmdLine
{
  KeyedCmdLine() {}

public:
  using basicstr = std::basic_string<C>;
  using tostream = std::basic_ostream<C>;

  static KeyedCmdLine Empty(const basicstr& prefix = StrToBasicstr("--"),
                            const basicstr& splitter = StrToBasicstr("="))
  {
    KeyedCmdLine k;
    k.prefix_ = prefix;
    k.splitter_ = splitter;
    return k;
  }

  KeyedCmdLine(
    int argc, C* argv[],
    const basicstr& prefix = StrToBasicstr("--"),
    const basicstr& splitter = StrToBasicstr("="))
    :
    prefix_(prefix),
    splitter_(splitter)
  {
    Construct(argc, argv, prefix, splitter);
  }

  KeyedCmdLine(
    const basicstr& program_path,
    const StringMap& named_args,
    const StringVector& unnamed_args,
    const basicstr& prefix = StrToBasicstr("--"),
    const basicstr& splitter = StrToBasicstr("="))
    :
    prefix_(prefix),
    splitter_(splitter),
    prog_path_(program_path),
    named_args_(named_args),
    unnamed_args_(unnamed_args)
  {
  }

  KeyedCmdLine(
    const char* raw_cmdline,
    const basicstr& prefix = StrToBasicstr("--"),
    const basicstr& splitter = StrToBasicstr("="))
    :
    prefix_(prefix), splitter_(splitter)
  {
    ParsedCommandLine pcl(raw_cmdline);
    if (pcl.GetParserRet() == 0) {
      if (pcl.GetArgc() != 0) {
        // Construct expects at least argv[0]
        Construct(pcl.GetArgc(), pcl.GetArgv(), prefix, splitter);
      }
    }
  }

  bool HasNamedArg(const basicstr& key) const { return named_args_.find(key) != named_args_.end(); }

  const basicstr& GetProgramPath() const { return prog_path_; }
  void SetProgramPath(const basicstr& path) { prog_path_ = path; }

  std::map<basicstr, basicstr>& GetNamedArgs() { return named_args_; } // non-const cuz ConfigFromDict can consume it
  std::vector<basicstr>& GetUnnamedArgs() { return unnamed_args_;}
  
  // Note: Keys can't contain spaces
  // Note: *** ideal matching not guaranteed! ***
  void Textualize(std::stringstream& ss, bool print_prog_name = true);
  void Textualize(std::string& out_text, bool print_prog_name = true);
  std::string Textualize(bool print_prog_name = true);

  // helper
  static void PrintUnknownArgsLeft(tostream& stm,
    const std::map<basicstr, basicstr>& named_args,
    basicstr line_prefix = StrToBasicstr(""));

  // Doesn't compare prefix_ and splitter_
  bool operator==(const KeyedCmdLine& r) const {
    return
      r.named_args_ == this->named_args_ &&
      r.unnamed_args_ == this->unnamed_args_ &&
      r.prog_path_ == this->prog_path_;
  }
  bool operator!=(const KeyedCmdLine& r) const {
    return !operator==(r);
  }

  // GCC error if private
  static basicstr StrToBasicstr(const std::string& str) { return basicstr(str.begin(), str.end()); }

private:
  void Construct(int argc, C* argv[],
                 const basicstr& prefix,
                 const basicstr& splitter)
  {
    Parse(argc, argv, prefix, splitter);
  }
  void Parse(int argc, C* argv[], const basicstr& prefix, const basicstr& splitter);
  void ProcessNamedArg(const basicstr& keyval, const basicstr& splitter);
  void ProcessUnnamedArg(const basicstr& arg);

private:
  basicstr prefix_;
  basicstr splitter_;
  basicstr prog_path_;
  std::map<basicstr, basicstr> named_args_;
  std::vector<basicstr> unnamed_args_;
};

}}


