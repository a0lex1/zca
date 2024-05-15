#pragma once

// C99 log prints only to system log
// XLOG_CURRENT_SINK not used

#include "co/xlog/severities.h"

#ifndef CO_XLOG_DISABLE

#ifdef __cplusplus
extern "C" {
#endif


  // rethink
//void xlog_init_system_log();
//void xlog_free_system_log();

void _logprint(
    int verb, const char* modname, const char* file,
    int line, const char* function, const char* fmt, ...);

#ifdef __cplusplus
}
#endif


#define logprint(sev, fmt, ...) \
  _logprint(sev, "", __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
namespace co { namespace xlog { class CLoggerSink; } }
#endif

#else //CO_XLOG_DISABLE

static inline void _XLogNop() { }

// ***************************************************************
// TODO: stripped !?!?!?
// ***************************************************************
#define logprint(...) _XLogNop()
#define xlog_init_system_log(...) _XLogNop()
#define xlog_free_system_log(...) _XLogNop()

#endif //CO_XLOG_DISABLE
