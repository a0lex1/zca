#pragma once

#include <map>
#include <string>

namespace netshell {

using NsFlag = unsigned;

static const NsFlag fCanNotHaveBody = 1; // mutually exclusive
static const NsFlag fCanHaveBody = 2;

using NsStatusDescriptorTable = std::map<int, std::pair<std::string, NsFlag>>;


}
