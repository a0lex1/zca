#include <boost/exception/diagnostic_information.hpp>

#include <string>
#include <sstream>

// we're just base, but this is just helper .h file
#include "co/xlog/xlog.h"

namespace co {

static inline void PrintBoostDiagnosticInformation(const std::exception& e,
                                                   const char* funcname, // optional
                                                   std::ostream& os) {

  return;

  if (funcname != nullptr) {
    os << "[DG] Excp in func << funcname: " << funcname << "() >>\n";
  }
  os
    << "[DG] Boost diagnostic information >>\n"
    << boost::diagnostic_information(e)
    << "\n";
}

static inline void PrintBoostDiagnosticInformationXlog(const std::exception& e) {
  std::stringstream o;
  PrintBoostDiagnosticInformation(e, nullptr, o);
  syslog(_WARN) << o.str() << "\n";
}

}


