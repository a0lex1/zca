#include "./parse_scheme_address.h"

using namespace std;

namespace co {
namespace net {

bool parse_scheme_address(const string& scheme_address,
                          string& scheme,
                          string& address)
{
  size_t n = scheme_address.find("://");
  if (n == string::npos) {
    return false;
  }
  scheme = scheme_address.substr(0, n);
  address = scheme_address.substr(n+3);
  return true;
}

}}

