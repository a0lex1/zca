#include "co/base/cmdline/keyed_cmdline.h"
#include "co/base/strings.h"

using namespace std;

namespace co {
namespace cmdline {

template <typename C>
void KeyedCmdLine<C>::ProcessNamedArg(const basicstr& keyval, const basicstr& splitter) {
  const size_t splitter_len = splitter.length();
  basicstr key, value;
  size_t splitter_pos = keyval.find(splitter);
  if (splitter_pos == string::npos) {
    key = keyval;
    value.clear();
  } else {
    key = keyval.substr(0, splitter_pos);
    value = keyval.substr(splitter_pos + splitter_len);
  }
  named_args_[key] = value;
}

template <typename C>
void KeyedCmdLine<C>::ProcessUnnamedArg(const basicstr& arg) {
  unnamed_args_.push_back(arg);
}

template <typename C>
void KeyedCmdLine<C>::Parse(int argc, C* argv[], const basicstr& prefix, const basicstr& splitter) {
  const size_t preflen = prefix.length();
  for (int i = 0; i < argc; i++) {
    const basicstr& cur_arg = argv[i];
    if (!prefix.compare(cur_arg.substr(0, preflen))) {
      ProcessNamedArg(cur_arg.substr(preflen), splitter);
    } else {
      ProcessUnnamedArg(cur_arg);
    }
  }
  if (!unnamed_args_.empty()) {
    prog_path_ = unnamed_args_.front();
    unnamed_args_.erase(unnamed_args_.begin());
  }
}

template <typename C /*= char*/>
void co::cmdline::KeyedCmdLine<C>::PrintUnknownArgsLeft(tostream& stm,
  const map<basicstr, basicstr>& named_args,
  basicstr line_prefix /*= StrToBasicstr("")*/)
{
  for (const auto& it : named_args) {
    stm << line_prefix << it.first << "\n";
  }
}

// We don't support wchar_t now, half of code doesn't support it (MakeRawArgv, ParseCmdLineToArgv)
//template class KeyedCmdLine<wchar_t>;

// ---------------------------

template <typename C>
void KeyedCmdLine<C>::Textualize(stringstream& ss, bool print_prog_name) {
  size_t i;
  bool need_space = false;

  // Program path (argv[0])
  if (print_prog_name) {
    if (prog_path_.length()) {
      string pn(prog_path_);
      if (strchr(pn.c_str(), ' ')) {
        // -= Escape =-
        pn = string("\"") + pn + "\"";
      }
      ss << pn;
      need_space = true;
    }
  }

  // Unnamed args
  i = 0;
  for (string uv : unnamed_args_) {
    if (need_space) {
      ss << " ";
      need_space = false;
    }
    if (i) {
      ss << " ";
    }
    if (strchr(uv.c_str(), ' ')) {
      // -= Escape =-
      uv = string("\"") + uv + "\"";
    }
    ss << uv;
    i += 1;
  }
  if (i != 0) {
    need_space = true;
  }

  // Named args
  i = 0;
  for (const auto& narg : named_args_) {
    if (need_space) {
      ss << " ";
      need_space = false;
    }
    string k(narg.first);
    string v(narg.second);
    if (i) {
      ss << " ";
    }
    DCHECK(!strchr(k.c_str(), ' '));
    ss << prefix_ << k;
    if (v.length()) {
      if (strchr(v.c_str(), ' ')) {
        // -= Escape =-
        v = string("\"") + v + "\"";
      }
      ss << splitter_ << v;
    }
    i += 1;
  }
  if (i != 0) {
    need_space = true;
  }
}

template <typename C>
void KeyedCmdLine<C>::Textualize(string& str, bool print_prog_name)
{
  stringstream ss;
  Textualize(ss, print_prog_name);
  str = ss.str();
}

template <typename C>
string KeyedCmdLine<C>::Textualize(bool print_prog_name)
{
  stringstream ss;
  Textualize(ss, print_prog_name);
  return ss.str();
}


template <typename ... T > using basicstr = typename KeyedCmdLine<T...>::basicstr;
template class KeyedCmdLine<char>;
//template void KeyedCmdLine<char>::Textualize(string& str, bool print_prog_name);


}}


