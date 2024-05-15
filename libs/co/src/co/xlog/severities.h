#pragma once

#include "co/xlog/severity_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SevTableEntry {
  int sev;
  const char* const _name;						//end then he raped me...
  const char* const display_tag;
} SevTableEntry;

extern const SevTableEntry g_severity_table[];
extern const int g_severity_table_count;

#ifdef __cplusplus
}
#endif





