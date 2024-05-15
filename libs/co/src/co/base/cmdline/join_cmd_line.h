#pragma once

#include "co/common.h"

namespace co {
namespace cmdline {

std::string JoinCmdLine(int argc, char* argv[]);
std::string JoinCmdLine(const StringVector& args, size_t start_from=0);


}}

