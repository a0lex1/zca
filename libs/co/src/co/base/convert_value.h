#pragma once

#include "co/net/endpoint.h"
#include "co/net/asio_endpoint_from_string.h"

namespace co {

bool ConvertValue(const std::string& str, uint16_t& val, int radix = 10);
bool ConvertValue(const std::string& str, uint32_t& val, int radix = 10);
bool ConvertValue(const std::string& str, int32_t& val, int radix = 10);
bool ConvertValue(const std::string& str, uint64_t& val, int radix = 10);
bool ConvertValue(const std::string& str, bool& val);
bool ConvertValue(const std::string& str, ip_address& val);
bool ConvertValue(const std::string& str, tcp_endpoint& val);
bool ConvertValue(const std::string& str, co::net::TcpEndpoint& val);
bool ConvertValue(const std::string& str, std::string& val);
bool ConvertValue(const std::string& str, StringMap& val);

// In Visual Studio x64  --  size_t& and uint64_t& are same types
// So can't have two overridden same funcs. I ripped it off.
//bool ConvertValue(const std::string& str, size_t& val, int radix = 10);

}
