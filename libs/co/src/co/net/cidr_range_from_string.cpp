#include "co/net/cidr_range_from_string.h"

#include <boost/algorithm/string.hpp>
#include <vector>

using namespace std;

namespace co {
namespace net {

bool cidr_range_from_string(cidr_range& rn, const string& input_string) {

  using boost::asio::ip::address;
  using boost::system::error_code;

  vector<string> x;
  boost::split(x, input_string, [](char c) { return c == '/'; });
  if (x.size() != 2) {
    return false; // missing '/M_SK'
  }
  try {
    uint32_t t = boost::lexical_cast<uint32_t>(x[1]);
    if (t > 0xff) {
      return false; // /M_SK is too big
    }
    rn.network_bits = static_cast<uint8_t>(t);
  }
  catch (boost::bad_lexical_cast&) {
    return false;
  }

  vector<string> y;
  boost::split(y, x[0], [] (char c) { return c == '-'; });
  if (y.size() == 2) {
    address a1, a2;
    error_code ec;
    a1 = address::from_string(y[0], ec);
    if (!ec) {
      a2 = address::from_string(y[1], ec);
      if (!ec) {
        rn.range = address_range(a1, a2);
        return true;
      }
    }
  }
  return false;
}

}}
