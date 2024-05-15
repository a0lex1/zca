#include "cc/cc_proto/fire_message.h"
#include "co/base/bin_reader.h"
#include "co/base/bin_writer.h"

using namespace std;

namespace cc {
namespace cc_proto {

bool FireMessage::SerializeBody(co::BinWriter& writer) const {
  return true;
}

bool FireMessage::UnserializeBody(co::BinReader& reader) {
  if (reader.BytesLeft() != 0) { // fire msg must be empty
    return false;
  }
  return true;
}

bool FireMessage::CompareBody(const ProtoMessage& r) const {
  return true;
}

}}


