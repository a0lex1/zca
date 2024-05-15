/////////////////////////////
// rewrite this when I need this
/////////////////////////////

#include "co/xlog/xlog_print.h"
#include "co/xlog/xlog.h"
#include <stdarg.h>

using namespace co::xlog;

#ifndef CO_XLOG_DISABLE

namespace co {
  namespace xlog {


  }
}

extern "C"
void _logprint(
  int sev,
  const char* modname,
  const char* file, int line, const char* func, const char* fmt, ...)
{
  DCHECK(co::xlog::detail::gsysloggerSink != nullptr);
  va_list vl;
  va_start(vl, fmt);
  co::xlog::CLoggerSink* p = co::xlog::detail::gsysloggerSink.get();
  p->print_vl(sev, modname, file, line, func, fmt, vl);
  va_end(vl);
}

#endif
