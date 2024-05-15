#include "co/net/cidr_range.h"

#include "co/net/address_calc.h"

using namespace std;

namespace co {
namespace net {

bool cidr_range::valid() const {
  if (!range.first_ip().is_v4() == range.last_ip().is_v4()) {
    return false;
  }
  if (!address_calc::valid_network_bits(range.first_ip().is_v4(),
                      network_bits))
  {
    return false;
  }
  return true;
}

string cidr_range::to_string() const {
  return
      range.to_string() + "/"
  + boost::lexical_cast<string>(
        static_cast<uint32_t>(network_bits));
}

}}
