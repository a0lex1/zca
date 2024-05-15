#pragma once

// this file is included in xlog.h ifCO_XLOG_DISABLE
// its for Release build

#include "co/common.h"

#include <boost/noncopyable.hpp>

namespace co {
namespace xlog {

#define XLOG_EMPTY() co::xlog::xlogPrinterObject()
#define Log(sev) XLOG_EMPTY()
#define syslog(sev) XLOG_EMPTY()

class xlogPrinterObject : public boost::noncopyable {
public:
  xlogPrinterObject() {
  }

  template <typename T>
  xlogPrinterObject& operator << (const T& val) {
    return *this;
  }
};

struct CLoggerOpts {};

class CLoggerDevice {};
class CLoggerOpenableDevice : public CLoggerDevice {};
class CLoggerSink {};
class CLoggerSubmoduleSink : public CLoggerSink {
public:
  CLoggerSubmoduleSink(Shptr<CLoggerSink>, const char*) { }
};
class CLoggerCallbackDevice : public CLoggerDevice {};
class CLoggerOstreamDevice : public CLoggerCallbackDevice {
public:
  CLoggerOstreamDevice(...) {}
};
class CLoggerOstreamRingDevice : public CLoggerCallbackDevice {
public:
  CLoggerOstreamRingDevice(...) {}
};
class CLoggerStdoutDevice : public CLoggerCallbackDevice {};
class CLogger : public CLoggerSink, public co::enable_shared_from_this<CLogger> {
public:
  Shptr<CLoggerSink> getSink() { return std::make_shared<CLoggerSink>(); }
};

static Shptr<CLogger> CreateLoggerWithDevices(const CLoggerOpts& opts,
  const std::vector<Shptr<CLoggerDevice>>& devs) 
{
  return nullptr;
}


}}




