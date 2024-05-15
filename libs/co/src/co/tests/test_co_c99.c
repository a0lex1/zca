#include "co/xlog/xlog_print.h"

void test_co_xlog_c99() {
  logprint(_TRACE, "C99 logprint() from C99 file: my pen is %d inches, its name %s\n", 411, "Little Johny");
  logprint(_DBG, "C99 logprint() from C99 file: my pen is %d inches, its name %s\n", 411, "Little Johny");
  logprint(_INFO, "C99 logprint() from C99 file: my pen is %d inches, its name %s\n", 411, "Little Johny");
  logprint(_ERR, "C99 logprint() from C99 file: my pen is %d inches, its name %s\n", 411, "Little Johny");
  logprint(_FATAL, "C99 logprint() from C99 file: my pen is %d inches, its name %s\n", 411, "Little Johny");
}
