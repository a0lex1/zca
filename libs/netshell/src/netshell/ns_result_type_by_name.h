#pragma once

#include "netshell/ns_result_type.h"

#include <string>

namespace netshell {

bool NsResultTypeByName(const std::string& type_name, NsResultType& t);

}
