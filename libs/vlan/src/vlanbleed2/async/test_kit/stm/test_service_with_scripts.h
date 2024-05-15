#pragma once

#include "vlanbleed2/async/test_kit/stm/script.h"

#include "co/async/service.h"

#include <map>
#include <vector>

// For testing Script pair scenarios on Service's
void TestServiceWithScriptPairs(StringMap& opts_dict,
                                co::async::Service& svc,
                                const std::vector<std::pair<Script, Script>>& pairs);

