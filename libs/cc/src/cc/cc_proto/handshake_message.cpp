#include "cc/cc_proto/handshake_message.h"
#include "cc/bot_id.h"
#include "co/base/bin_reader.h"
#include "co/base/bin_writer.h"

using namespace std;

namespace cc {
namespace cc_proto {

bool HandshakeMessage::SerializeBody(co::BinWriter& writer) const {

  if (!writer.WriteRaw(bot_id_.GetBytes(), BotId::kByteLength)) {
    return false;
  }

  if (!writer.WriteUint32(bot_ver_)) {
    return false;
  }

  if (!writer.WriteString(opaque_data_)) {
    return false;
  }

  return true;
}

bool HandshakeMessage::UnserializeBody(co::BinReader& reader) {
  BotId bot_id;
  char* bot_id_bytes = const_cast<char*>(bot_id.GetBytes());
  if (!reader.ReadFixedNumberOfPods(bot_id_bytes, BotId::kByteLength)) {
    return false;
  }

  uint32_t bot_ver;
  if (!reader.ReadUint32(bot_ver)) {
    return false;
  }

  string opaque_data;
  if (!reader.ReadString(opaque_data)) {
    return false;
  }

  if (reader.BytesLeft() != 0) {
    // We have an unexpected tail.
    return false;
  }

  bot_id_ = bot_id;
  bot_ver_ = bot_ver;
  opaque_data_ = opaque_data;
  return true;
}

bool HandshakeMessage::CompareBody(const ProtoMessage& r) const {
  return
    this->bot_id_ == static_cast<const HandshakeMessage&>(r).bot_id_ &&
    this->bot_ver_ == static_cast<const HandshakeMessage&>(r).bot_ver_;
}

}}

