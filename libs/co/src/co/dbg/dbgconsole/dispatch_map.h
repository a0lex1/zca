#pragma once

#include "co/common.h"

#include <map>
#include <string>

namespace co {
namespace dbg {
namespace dbgconsole {

using DispatchFunc = Func<void(const std::string&, std::string&, bool& abort_read)>;
using DispatchMap = std::map<std::string, DispatchFunc>;


}}}
