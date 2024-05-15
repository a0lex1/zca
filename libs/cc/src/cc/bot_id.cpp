#include "cc/bot_id.h"

#include "co/base/strings.h"
#include "co/base/rand_gen.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/random.hpp>

using namespace co;
using namespace std;

namespace cc {

BotId::BotId()
{
  Clear();
}

BotId::BotId(unsigned number)
{
  data.i64.hi = 0;
  data.i64.lo = number;
}

BotId::BotId(uint64_t hi, uint64_t low) {
  data.i64.hi = hi;
  data.i64.lo = low;
}

const char* BotId::GetBytes() const
{
  return data.bytes;
}

cc::BotId BotId::FromUint(unsigned number)
{
  return BotId(number);
}

cc::BotId BotId::FromUints(uint64_t hi, uint64_t low)
{
  return BotId(hi, low);
}

string BotId::ToStringRepr() const
{
  DCHECK(sizeof(data.bytes) == 16);
  DCHECK(sizeof(data) == 16);

  string s;
  s.reserve(32);

  string a = string_from_uint64(data.i64.hi, 16/*radix*/);
  string b = string_from_uint64(data.i64.lo, 16/*radix*/);

  s = string(16 - a.length(), '0') + a + string(16 - b.length(), '0') + b;
  return s;
}

bool BotId::FromStringRepr(const std::string& text_repr)
{
  if (text_repr.length() != 32) {
    // Width must be 16 since we set it in ToStringRepr
    return false;
  }
  string s_hi(text_repr.substr(0, 16));
  string s_lo(text_repr.substr(16));
  if (!string_to_uint64(s_hi, data.i64.hi, 16/*radix*/)) {
    // corrupting state on error
    return false;
  }
  if (!string_to_uint64(s_lo, data.i64.lo, 16/*radix*/)) {
    return false;
  }
  return true;
}

void BotId::Clear()
{
  data.i64.hi = 0;
  data.i64.lo = 0;
}

bool BotId::operator==(const BotId& r) const
{
  return this->data.i64.hi == r.data.i64.hi && this->data.i64.lo == r.data.i64.lo;
}

bool BotId::operator!=(const BotId& r) const
{
  return !operator==(r);
}

}


