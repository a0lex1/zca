#include "cc/cc_proto/skipme_message.h"

namespace cc {
namespace cc_proto {

bool SkipMeMessage::CompareBody(const ProtoMessage &) const
{
  return true;
}

bool SkipMeMessage::SerializeBody(co::BinWriter &writer) const
{
  return true;
}

bool SkipMeMessage::UnserializeBody(co::BinReader &reader)
{
  return true;
}


}}

