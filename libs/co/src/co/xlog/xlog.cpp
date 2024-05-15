#include "co/xlog/global_data.h"
#include "co/xlog/xlog.h"
#include "co/xlog/define_logger_sink.h"
#include "co/base/strings.h"

#include <cstdarg>
#include <iostream>
#include <atomic>
#include <thread>

#include <boost/interprocess/sync/scoped_lock.hpp>

using namespace std;

namespace co {
namespace xlog {

static const size_t kPrintBufSize = 20000;

namespace detail { Shptr<CLoggerSink> gsysloggerSink; }

#ifdef CO_XLOG_DISABLE
Shptr<CLogger> CreateLoggerWithDevices(const char* logfmt, int logopts,
  vector<Shptr<CLoggerDevice>> devs) {
  return Shptr<CLogger>(nullptr);
}
#endif

#ifndef CO_XLOG_DISABLE

xlogPrinterObject::xlogPrinterObject(Shptr<CLoggerSink> sink,
                                     int sev,
                                     const char *modname,
                                     const char *file,
                                     int line,
                                     const string& func)
  :
  sink_(sink), sev_(sev), modname_(modname), file_(file), line_(line), func_(func)
{
}

xlogPrinterObject::~xlogPrinterObject() {
  if (sink_ == nullptr) {
    // Silently return
    // This can be uninitialized |syslogSink|
    return;
  }
  sink_->print(
    sev_,
    modname_.c_str(), file_.c_str(), line_, func_.c_str(),
    "%s",
    buf_.str().c_str());
}

string FixLongLambdaNames(const char* func) {
  // FixLongLambdaNames()
  // Fix xxx::<lambda_f59fd123cdb3b356a297a3de67af801d>::operator() to
  //     xxx::<lambda_~>
  char* p = (char*)strstr(func, "<lambda_");
  if (p) {
    char* p2 = (char*)strstr(&p[8], "::operator ()");
    if (p2) {
      string ret;
      ret.append(func, p - func + 8);
      ret.append("~>");
      return ret;
    }
  }
  return func;
}

string FixLongAnonymousNames(const char* func) {
  // FixLongLambdaNames()
  // Fix `anonymous-namespace': to
  //     `
  return string_replace_all_once(func, "`anonymous-namespace'", "`");
}


// 
// Management -----------------------------------------------

static string MakeSeverityTag(int sev) {
  for (int i=g_severity_table_count-1; i>=0; i--) {
    if (sev == g_severity_table[i].sev) {
      return g_severity_table[i].display_tag;
    }
    else {
      if (sev > g_severity_table[i].sev) {
        int ind_ofs = sev - g_severity_table[i].sev;
        return string_printf("%s+%d", g_severity_table[i].display_tag, ind_ofs);
      }
    }
  }
  return "?";
}

bool SeverityFromString(const string& strsev, int& sev) {
  // Here we can add support for +N parsing
  // acceptor:trace;syslog:info
  //                       info
  for (int i = g_severity_table_count - 1; i >= 0; i--) {
    if (strsev == g_severity_table[i]._name) {
      sev = g_severity_table[i].sev;
      return true;
    }
  }
  return false;
}

const CLoggerOpts& CLoggerSink::getOpts() const {
  return *opts_.get();
}

void CLoggerSink::print(int sev, const char *modname, const char *file,
            int line, const char *func, const char *fmt, ...)
{
  va_list vl;
  va_start(vl, fmt);
  print_vl(sev, modname, file, line, func, fmt, vl);
  va_end(vl);
}

void CLoggerSink::print_vl(int sev, const char* modname, const char* file,
  int line, const char* func, const char* fmt, va_list vl)
{
  if (sev < GetOpts().min_severity) {
    return;
  }
  string sevname = MakeSeverityTag(sev);

  std::string user_msg(kPrintBufSize+1, '\0');
  vsnprintf((char*)user_msg.c_str(), kPrintBufSize, fmt, vl);

  string modtag;
  if (modname[0] != '\0') {
    modtag = " {";
    modtag += modname;
    modtag += "}";
  }

  auto NumSpacesToTabTid = [](unsigned int tid) {
    int nchars = 0;
    if (tid < 9999) {
      nchars++;
      if (tid < 999) {
        nchars++;
        if (tid < 99) {
          nchars++;
          if (tid < 9) {
            nchars++;
          }
          // if more, we don't give a funk
        }
      }
    }
    return nchars;
  };

  stringstream prefix;
  prefix << "[" << sevname << "]" << GetOpts().sep_char;
  if (GetOpts().format_flags & fLogPrintTid) {
#ifdef _WIN32
    uint32_t dwTid = GetCurrentThreadId();
    prefix << "(TID " << dwTid << ") ";
//#elif (defined(__APPLE__))
#else
    // both apple and linux
    uint64_t dwTid = reinterpret_cast<uint64_t>(pthread_self());
    prefix << "(TID " << hex << dwTid << ") ";
#endif

    auto nst(NumSpacesToTabTid(dwTid));

    for (auto i = 0; i < nst; i++) {
      prefix << " ";
    }
  }
  if (modname[0] != '\0') {
    prefix << modtag << " ";
  }

  if (GetOpts().format_flags & fLogPrintFunc) {
    string new_func;
    if (GetOpts().fix_long_lambda_names) {
      // lambda long names not cool
      new_func = FixLongLambdaNames(func);
    }
    else {
      new_func = func;
    }
    if (GetOpts().fix_long_anonymous_names) {
      // fix `anonymous namespace` in function names
      new_func = FixLongAnonymousNames(new_func.c_str());
    }

    prefix << new_func << "(): ";

    if (GetOpts().pad_func_name) {
      // enable padding
      // RACECONDS HERE, but we don't care
      // maximum impact is scary messed up log, not remote code execution
      if (new_func.length() < global_data::g_cur_func_name_column_padding) {
        size_t space_count = global_data::g_cur_func_name_column_padding - new_func.length();
        prefix << string(space_count, ' ');
      }
      else {
        if (new_func.length() > global_data::g_cur_func_name_column_padding) {
          global_data::g_cur_func_name_column_padding = new_func.length();
        }
      }
    }
  }

  stringstream ss;
  //string_replace_all_once(user_msg, "\r", " ");
  StringVector parts;
  string_split(user_msg, "\n", parts);
  if (parts.size() != 0) {
    if (parts[parts.size() - 1] == "") {
      parts.erase(parts.end() - 1);
    }
    if (parts.size() > 1) {
      // For multiline output, begin with newline.
      ss << prefix.str() << "\n" << parts[0] << "\n";
    }
    else {
      ss  << prefix.str() << parts[0] << "\n";
    }
    if (parts.size() > 1) {
      for (size_t i=1;i<parts.size(); i++) {
#ifdef CO_XLOG_ADD_MULTILINES
        ss << prefix.str() << parts[i] << "\n";
#endif
        ss << parts[i] << "\n";
      }
    }
  }
  output_line(ss.str().c_str());
}

const CLoggerOpts& CLoggerSink::GetOpts() const
{
  return *opts_.get();
}

CLoggerSink::CLoggerSink(const CLoggerOpts& opts)
  :
  opts_(make_unique<CLoggerOpts>(opts))  
{

}

CLoggerSubmoduleSink::CLoggerSubmoduleSink(Shptr<CLoggerSink> parent, const char *thismodname)
  :
  CLoggerSink(parent->getOpts()),
  parent_(parent),
  thismodname_(thismodname)
{
}

void CLoggerSubmoduleSink::print_vl(int sev, const char *modname, const char *file, int line, const char *func, const char *fmt, va_list vl)
{
  string modn;
  if (strlen(modname) == 0) {
    modn = thismodname_;
  }
  else {
    modn = thismodname_ + "::" + modname;
  }
  parent_->print_vl(sev, modn.c_str(), file, line, func, fmt, vl);
}

CLoggerCallbackDevice::CLoggerCallbackDevice(cbk_func cbk): cbk_(cbk) {
  mutex_ = make_shared<boost::mutex>();
}

void CLoggerCallbackDevice::write_text(const char *line) {
  boost::mutex::scoped_lock lock(*mutex_.get());
  cbk_(line);
}

#ifdef _WIN32
CLoggerDbgoutDevice::CLoggerDbgoutDevice()
  : CLoggerCallbackDevice([](const char* msg) {
  OutputDebugStringA(msg);
    })
{
}
#endif

void CLogger::output_line(const char *line) {\
  for (auto it=devs_.begin(); it!=devs_.end(); it++) {
    (*it)->write_text(line);
  }
}

Shptr<CLogger> CreateLoggerWithDevices(const CLoggerOpts& opts,
  const vector<Shptr<CLoggerDevice>>& devs)
{
  CLogger* logger = new CLogger(opts);
  for (auto dev : devs) {
    logger->addDevice(dev);
  }
  return Shptr<CLogger>(logger);
}

// ----------------------------------------------------------------------------------------

CLoggerOstreamRingDevice::CLoggerOstreamRingDevice(ostream& stm, size_t ring_size,
  bool do_flush, bool binary_mode)
  :
  CLoggerCallbackDevice(co::bind(&CLoggerOstreamRingDevice::Callback, this, _1)),
  stm_(stm), ring_size_(ring_size), do_flush_(do_flush), add_marker_(false),
  notgood_msg_written_(false), nospace_msg_written_(false), binary_mode_(binary_mode)
{
  DCHECK(stm.good());
}

CLoggerOstreamRingDevice::CLoggerOstreamRingDevice(Shptr<ostream> shared_stm, size_t ring_size, bool do_flush, bool binary_mode)
  :
  CLoggerOstreamRingDevice(*shared_stm.get(), ring_size, do_flush, binary_mode)
{
  shared_stm_ = shared_stm; // keep alive
}

void CLoggerOstreamRingDevice::Callback(const char* _line) {
  if (!stm_.good()) {
    if (!notgood_msg_written_) {
      notgood_msg_written_ = true;
      cout << "********** CLoggerOstreamRingDevice: !ofstream->good\n";
    }
    return;
  }
  bool fail_if_no_space = false;
check_again:
  string line(_line);
  if (add_marker_) {
    line += kRingbufOlddataMessage;
  }
  auto cur_pos = static_cast<size_t>(stm_.tellp());
  size_t space_left = ring_size_ - cur_pos;
  if (space_left < line.length()) {
    if (fail_if_no_space) {
      if (!nospace_msg_written_) {
        nospace_msg_written_ = true;
        cout << "********** CLoggerOstreamRingDevice: no free space\n";
      }
      return;
    }
    // Fill the reset of the space
    for (size_t i = 0; i < space_left; i++) {
      stm_.write(".", 1);
    }
    // Start from beginning and add marker from now on
    stm_.seekp(0);
    add_marker_ = true;
    fail_if_no_space = true;
    goto check_again;
  }
  stm_ << line;
  if (add_marker_) {
    // Seek back: Next write will rewrite the marker and write its own
    int extra_chars;
    if (!binary_mode_) {
      extra_chars = 2;
    }
    else {
      extra_chars = 0;
    }
    stm_.seekp(
      -static_cast<int>(kRingbufOlddataMessage.length()) - extra_chars,
      ios_base::cur);
  }
  if (do_flush_) {
    stm_.flush();
  }
  // TODO: check if disk no free space!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! <-- DO I REALLY NEED IT?
  DCHECK(!stm_.eof());
}


CLoggerOstreamDevice::CLoggerOstreamDevice(ostream& stm,
                                           size_t max_size,
                                           bool do_flush)
  :
  CLoggerCallbackDevice(co::bind(&CLoggerOstreamDevice::Callback, this, _1)),
  stm_(stm), max_size_(max_size), do_flush_(do_flush),
  maxsize_msg_written_(false)
{

}

CLoggerOstreamDevice::CLoggerOstreamDevice(Shptr<ostream> shared_stm, size_t max_size, bool do_flush) :
  CLoggerOstreamDevice(*shared_stm.get(), max_size, do_flush)
{
  shared_stm_ = shared_stm; // keep alive
}

void CLoggerOstreamDevice::Callback(const char* _line) {
  if (max_size_ != kUnlimitedFileSize) {
    auto cur_pos = static_cast<size_t>(stm_.tellp());
    if (cur_pos + strlen(_line) > max_size_) {
      if (!maxsize_msg_written_) {
        maxsize_msg_written_ = true;
        cout << "********** CLoggerOstreamDevice: max size " << max_size_ << " reached\n";
      }
      return;
    }
  }
  //printf("%s", _line);
  stm_ << _line;
  if (do_flush_) {
    stm_.flush();
  }
}

Shptr<CLoggerSink> SetsyslogSink(Shptr<CLoggerSink> sink) {
  auto prev_sink = detail::gsysloggerSink;
  detail::gsysloggerSink = sink;
  return prev_sink;
}

void EnumLogSinks(map<string, Shptr<CLoggerSink>>& sinks) {
  sinks.clear();
  for (auto& it : detail::xlogVarTable::get().getSinkPtrMap()) {
    sinks.insert(pair<string, Shptr<CLoggerSink>>(it.first, *it.second));
  }
}

void PrintLogSinks(ostream& stm, const map<string, Shptr<CLoggerSink>>& sinks, int num_spaces) {
  for (auto& it : sinks) {
    for (int i = 0; i < num_spaces; i++) {
      stm << " ";
    }
    stm << it.first << " => ";
    if (it.second == nullptr) {
      stm << "NULL";
    }
    else {
      PrintLoggerOpts(stm, it.second.get()->getOpts());
    }
    stm << "\n";
  }
}

void PrintLoggerOpts(ostream& stm, const CLoggerOpts& opts) {
  if (opts.format_flags & fLogPrintTid) {
    stm << "[PrintThreadId] ";
  }
  if (opts.format_flags & fLogPrintFunc) {
    stm << "[PrintFunc]";
  }
  stm << MakeSeverityTag(opts.min_severity);
}

string GetShortenFuncName(const string& __function__) {
  StringVector parts;
  size_t pos = __function__.rfind("::");
  if (pos == string::npos) {
    return __function__;
  }
  return __function__.substr(pos+2);
}

#endif //CO_XLOG_DISABLE

}}


