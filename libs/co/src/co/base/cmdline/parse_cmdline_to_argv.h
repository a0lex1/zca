#pragma once

#include <vector>
#include <string>

namespace co {
namespace cmdline {

// Supports both ' and "
bool ParseCmdlineToArgv(const char* cmdline, std::vector<std::string>& argv);

}}

