#pragma once

#include "co/xlog/severity_from_string.h"
#include "co/xlog/severities.h"
#include "co/common.h"

#include <sstream>
#include <boost/noncopyable.hpp>
#include <fstream>
#include <ostream>
#include <iostream>
#include <vector>
#include <functional>
#include <algorithm>
#include <atomic>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

// #DebugFeatures
//#define CO_XLOG_ADD_MULTILINES

#ifdef CO_XLOG_DISABLE

#include "co/xlog/xlog_empty_stub.h"

#else

namespace co {
namespace xlog {

class CLogger;
class CLoggerSink;

std::string GetShortenFuncName(const std::string& __function__);

static const std::string kRingbufOlddataMessage = "\n<RINGBUF.OLD>\n";

// No __FUNCTION__ on windows, use Shortener like on linux
#define _XLOG_OBJ(sink, sev) co::xlog::xlogPrinterObject(sink,\
                                                         sev, \
                                                         "", \
                                                         __FILE__, \
                                                         __LINE__, \
                                                         co::xlog::GetShortenFuncName(__FUNCTION__))

#define Log(sev) _XLOG_OBJ(XLOG_CURRENT_SINK, sev)
#define syslog(sev) _XLOG_OBJ(co::xlog::detail::gsysloggerSink, sev)

std::string FixLongLambdaNames(const char* func);
std::string FixLongAnonymousNames(const char* func);

class xlogPrinterObject: public boost::noncopyable {
public:
  xlogPrinterObject(
    Shptr<CLoggerSink> sink, int sev, const char* modname, const char* file, int line,
    const std::string& func);

  ~xlogPrinterObject();
  
  template <typename T>
  xlogPrinterObject& operator<<(const T& val)
  {
    std::stringstream buf;
    buf_ << val;
    return *this;
  }

private:
  Shptr<CLoggerSink> sink_;
  int sev_;
  std::string modname_;
  std::string file_;
  int line_;
  std::string func_;
  std::stringstream buf_;
};

// Management -----------------------------

class CLoggerDevice {
public:
  virtual ~CLoggerDevice() { }
  virtual void write_text(const char* line) = 0;
};

class CLoggerEmptyDevice : public CLoggerDevice {
public:
  virtual ~CLoggerEmptyDevice() = default;
  virtual void write_text(const char* line) override final {}
};

struct CLoggerOpts;

class CLoggerSink {
public:
  virtual ~CLoggerSink() { }

  const CLoggerOpts& getOpts() const;

  void print(
      int sev,
      const char* modname, const char* file, int line,
      const char* func, const char* fmt, ...);

  virtual void print_vl(
      int sev,
      const char* modname, const char* file, int line,
      const char* func, const char* fmt, va_list vl);

protected:
  const CLoggerOpts& GetOpts() const;

private:
  virtual void output_line(const char* line) = 0;
  friend class CLogger;
  friend class CLoggerSubmoduleSink;
  CLoggerSink(const CLoggerOpts& opts);

private:
  Uptr<CLoggerOpts> opts_;
  //std::atomic<size_t> cur_func_name_column_padding_{ 0 }; // modified from any thread!//now global
};

class CLoggerSubmoduleSink: public CLoggerSink {
public:
  virtual ~CLoggerSubmoduleSink() { }

  CLoggerSubmoduleSink(Shptr<CLoggerSink> parent, const char* thismodname);

private:
  virtual void print_vl(
      int sev,
      const char* modname, const char* file, int line,
      const char* func, const char* fmt, va_list vl) override;

  virtual void output_line(const char* line) override {
    NOTREACHED();
  }

private:
  Shptr<CLoggerSink> parent_;
  std::string thismodname_;
};

class CLoggerCallbackDevice: public CLoggerDevice {
public:
  virtual ~CLoggerCallbackDevice() { }

  typedef Func<void(const char*)> cbk_func;

  CLoggerCallbackDevice(cbk_func cbk);

private:
  virtual void write_text(const char* line) override;
private:
  cbk_func cbk_;
  Shptr<boost::mutex> mutex_;
};

#ifdef _WIN32
// OutputDebugString
class CLoggerDbgoutDevice : public CLoggerCallbackDevice {
public:
  virtual ~CLoggerDbgoutDevice() = default;
  CLoggerDbgoutDevice();
};
#endif

enum FormatFlags {
  fLogPrintTid = 1,
  fLogPrintFunc = 2
};

struct CLoggerOpts {
  uint32_t format_flags{ fLogPrintFunc };
  int min_severity{ _INFO };
  bool fix_long_lambda_names{ true };
  char sep_char{ ' ' };
  bool pad_func_name{ true };
  bool fix_long_anonymous_names{ true };
};

/*
* class CLogger
* 
* Cool.
* 
*/
class CLogger:
  public CLoggerSink,
  public co::enable_shared_from_this<CLogger> {
public:
  virtual ~CLogger() { }

  // CLoggerSink has &opts reference, CLogger has a copy
  CLogger(const CLoggerOpts& opts, const std::vector<Shptr<CLoggerDevice>>& devs = {})
    :
    CLoggerSink(opts),
    devs_(devs)
  {
  }

  void addDevice(Shptr<CLoggerDevice> device) {
    devs_.push_back(device);
  }

  Shptr<CLoggerSink> getSink() {
    // unnecessary
    return std::static_pointer_cast<CLoggerSink>(shared_from_this());
  }

  //CLogger() { } // meaningless to create without shared_ptr cuz we're a sink, others ref us

private:
  virtual void output_line(const char* line) override;

private:
  std::vector<Shptr<CLoggerDevice>> devs_;
};

class CLoggerOstreamDevice : public CLoggerCallbackDevice {
public:
  virtual ~CLoggerOstreamDevice() = default;
  
  // for std::cout, etc.
  // |max_size| can be kUnlimitedFileSize
  CLoggerOstreamDevice(std::ostream& stm, size_t max_size, bool do_flush);
  // for file, etc.
  CLoggerOstreamDevice(Shptr<std::ostream> shared_stm, size_t max_size, bool do_flush);
  
private:
  void Callback(const char* _line);
  
private:
  std::ostream& stm_;
  size_t max_size_{ 0 };
  Shptr<std::ostream> shared_stm_;
  bool do_flush_{ false };
  bool maxsize_msg_written_{ false };
};

static const size_t kUnlimitedFileSize = -1;

class CLoggerStdoutDevice: public CLoggerOstreamDevice {
public:
  virtual ~CLoggerStdoutDevice() { }

  CLoggerStdoutDevice(bool do_flush = true)
    : CLoggerOstreamDevice(std::cout, kUnlimitedFileSize, do_flush)
  {
  }
};

class CLoggerOstreamRingDevice : public CLoggerCallbackDevice {
public:
  virtual ~CLoggerOstreamRingDevice() = default;

  // Need to know if the stream is opened in binary mode
  // std::stringstream is binary
  // std::ofstream for log opened in text mode by default
  //
  CLoggerOstreamRingDevice(std::ostream& stm, size_t ring_size, bool do_flush, bool binary_mode);
  CLoggerOstreamRingDevice(Shptr<std::ostream> shared_stm, size_t ring_size, bool do_flush, bool binary_mode);
  std::ostream& GetOstream() { return stm_; }

private:
  void Callback(const char* _line);

private:
  std::ostream& stm_;
  Shptr<std::ostream> shared_stm_;
  size_t ring_size_{ 0 };
  bool do_flush_{ false };
  bool add_marker_{ false };
  bool notgood_msg_written_{ false };
  bool nospace_msg_written_{ false };
  bool binary_mode_{ false };
};

// ----------------------------------------------------------------------------------------------

// Returns Shptr<> to previous sink
Shptr<CLoggerSink> SetsyslogSink(Shptr<CLoggerSink> sink);

Shptr<CLogger> CreateLoggerWithDevices(const CLoggerOpts& opts = CLoggerOpts(),
  const std::vector<Shptr<CLoggerDevice>>& devices = { make_shared<CLoggerStdoutDevice>() });

// ----------------------------------------------------------------------------------------------

void EnumLogSinks(std::map<std::string, Shptr<CLoggerSink>>&sinks);
void PrintLogSinks(std::ostream& stm, const std::map<std::string, Shptr<CLoggerSink>>& sinks, int num_spaces = 0);
void PrintLoggerOpts(std::ostream& stm, const CLoggerOpts& opts);

// ----------------------------------------------------------------------------------------------

namespace detail {
extern Shptr<CLoggerSink> gsysloggerSink;
}

}}

#endif //CO_XLOG_DISABLE


