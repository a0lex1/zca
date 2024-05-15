#include "cc/cc_proto/ping_message.h"
#include "co/base/bin_reader.h"
#include "co/base/bin_writer.h"

using namespace std;

namespace cc {
namespace cc_proto {

bool PingMessage::SerializeBody(co::BinWriter& writer) const {
  return true;
}

bool PingMessage::UnserializeBody(co::BinReader& reader) {
  if (reader.BytesLeft() != 0) { // ping msg must be empty
    return false;
  }
  return true;
}

bool PingMessage::CompareBody(const ProtoMessage& r) const {
  return true;
}

}}


