#if 0


#include <boost/stacktrace.hpp>
#include <sstream>

namespace co {

static void GetStackTraceIfDebug(std::string& text) {
  
  std::stringstream ss_stk;
#ifndef NDEBUG
  //ss_stk << boost::stacktrace::stacktrace();
#endif
  text = ss_stk.str();
}

}

#endif
