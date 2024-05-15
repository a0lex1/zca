#pragma once

#include <stdint.h>
#include <string>

namespace co {
namespace net {

/*
bool parse_host_port(const std::wstring& host_port,
   std::wstring& host, uint16_t& port);
*/

bool parse_host_port(const std::string& host_port,
   std::string& host, uint16_t& port);

}}
