#include "co/base/convert_value.h"
#include "co/base/strings.h"
#include "co/base/dict.h"

using namespace std;

namespace co {

bool ConvertValue(const string& str, uint16_t& val, int radix) {
  uint32_t val32;
  if (!ConvertValue(str, val32, radix)) {
    return false;
  }
  if (val32 > numeric_limits<uint16_t>::max()) {
    return false;
  }
  val = static_cast<uint16_t>(val32); // castrate
  return true;
}

bool ConvertValue(const string& str, uint32_t& val, int radix /*= 10*/)
{
  return string_to_uint(str, val, radix);
}

/*
bool ConvertValue(const string& str, size_t& val, int radix / *= 10* /)
{
  return string_to_size_t(str, val, radix);
}
*/

bool ConvertValue(const string& str, int32_t& val, int radix /*= 10*/)
{
  return string_to_int(str, val, radix);
}

bool ConvertValue(const string& str, uint64_t& val, int radix /*= 10*/)
{
  return string_to_uint64(str, val, radix);
}

bool ConvertValue(const string& str, bool& val)
{
  // --param without value means TRUE
  // --param=1 means TRUE
  // --param=0 means TRUE
  if (str == "") {
    val = true;
    return true;
  }
  if (str == "1") {
    val = true;
    return true;
  }
  if (str == "0") {
    val = false;
    return true;
  }
  return false;
}

bool ConvertValue(const string& str, ip_address& val)
{
  Errcode err;
  val = ip_address::from_string(str, err);
  return !err;
}

bool ConvertValue(const string& str, tcp_endpoint& val)
{
  Errcode err;
  net::EndpointFromIpPortStr(val, str, err);
  return !err;
}

bool ConvertValue(const string& str, net::TcpEndpoint& val)
{
  Errcode err;
  tcp_endpoint ep;
  co::net::EndpointFromIpPortStr(ep, str, err);
  if (!err) {
    val = net::TcpEndpoint(ep);
    return true;
  }
  return false;
}

bool ConvertValue(const string& str, string& val)
{
  val = str;
  return true;
}

bool ConvertValue(const string& str, StringMap& val)
{
  return ParseSingleLineDictAs(str, val);
}


}


