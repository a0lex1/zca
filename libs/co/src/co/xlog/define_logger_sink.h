#pragma once

#include "co/common.h"
#include <boost/noncopyable.hpp>

#ifndef CO_XLOG_DISABLE

#define DECLARE_XLOGGER_SINK(name, var) \
  extern Shptr<co::xlog::CLoggerSink> var;

#define DEFINE_XLOGGER_SINK(name, var) \
  Shptr<co::xlog::CLoggerSink> var; \
  static co::xlog::detail::xlogVarAdder __xlog_##var##_adder(name, var);

#else

#define DECLARE_XLOGGER_SINK(name, var)

#define DEFINE_XLOGGER_SINK(name, var)

#endif

namespace co {
namespace xlog {

class CLoggerSink;

namespace detail {

using SinkMap = std::map<std::string, Shptr<CLoggerSink>*>;

class xlogVarTable {
public:
  static xlogVarTable& get() {
    static xlogVarTable tbl;
    return tbl;
  }
  void addSinkPtr(const char* name, Shptr<CLoggerSink>& shptr);
  SinkMap& getSinkPtrMap();
private:
  xlogVarTable() {}
  xlogVarTable(const xlogVarTable&) {}
  ~xlogVarTable() {}
private:
  SinkMap shptrs_;
};

class xlogVarAdder : public boost::noncopyable {
public:
  xlogVarAdder(const char* name, Shptr<CLoggerSink>& shptr) {
    xlogVarTable::get().addSinkPtr(name, shptr);
  }
};
} // namespace detail


// For viewing in debugger
static const co::xlog::detail::xlogVarTable& _dbgLogVarTable(co::xlog::detail::xlogVarTable::get());

}}






