#include "co/net/cidr_ranges_from_string.h"
#include "co/net/cidr_range_from_string.h"

#include <boost/algorithm/string.hpp>

using namespace std;

namespace co {
namespace net {

bool cidr_ranges_from_string(
    const string&     input_string,
    vector<cidr_range>&   rv)
{
  vector<string> p;
  boost::split(p, input_string, [] (char c) { return c == ';'; });
  for (size_t i = 0; i < p.size(); i++) {
    cidr_range cr;
    if (!cidr_range_from_string(cr, p[i])) {
      return false;
    }
    rv.push_back(cr);
  }
  return true;
}


}}
