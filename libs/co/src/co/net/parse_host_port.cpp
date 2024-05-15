#include "co/net/parse_host_port.h"

#include "co/base/strings.h"

#include <boost/algorithm/string.hpp>

#include <vector>

using namespace std;

namespace co {
namespace net {

bool parse_host_port(const string& host_port, string& host, uint16_t& port)
{
  vector<string> parts;
  boost::split(parts, host_port, boost::is_any_of(":"));
  if (parts.size() != 2) {
    return false;
  }
  if (parts[0].empty()) {
    return false;
  }
  //unsigned short p = atoi(parts[1].c_str());
  unsigned short p;
  if (!string_to_uint16(parts[1].c_str(), p )) {
    return false;
  }
  host = parts[0];
  port = p;
  return true;
}

/*
bool parse_host_port(const wstring& host_port, wstring& host, uint16_t& port) {
  string _host;
  string s(host_port.begin(), host_port.end());
  if (parse_host_port(s, _host, port)) {
  host = wstring(_host.begin(), _host.end());
  return true;
  }
  return false;
}
*/

}}
