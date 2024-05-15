#pragma once

#include <string>
#include <map>

namespace co {
namespace xlog {
namespace configs {

#ifndef CO_XLOG_DISABLE
typedef std::map<std::string, int> SeverityMap;
struct ModuleSeverityMap {
  SeverityMap sev_map;
};
#endif

}}}
