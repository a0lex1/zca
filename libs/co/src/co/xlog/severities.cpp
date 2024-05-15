#include "co/xlog/severities.h"

#ifdef __cplusplus
extern "C" {
#endif

const SevTableEntry g_severity_table[] = {
  {_TRACE,    "trace",    "TRACE" },
  {_DBG,      "debug",    "DBG  " },
  {_INFO,     "info",     "INFO " },
  {_WARN,     "warn",     "WARN " },
  {_ERR,      "error",    "ERROR" },
  {_FATAL,    "fatal",    "FATAL" },
  {_MAXFATAL, "maxfatal", "MAXFATAL" }
};

const int g_severity_table_count = sizeof(g_severity_table) / sizeof(g_severity_table[0]);

#ifdef __cplusplus
}
#endif


