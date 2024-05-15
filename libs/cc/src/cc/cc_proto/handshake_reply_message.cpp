#include "cc/cc_proto/handshake_reply_message.h"
#include "co/base/bin_reader.h"
#include "co/base/bin_writer.h"

using namespace std;

namespace cc {
namespace cc_proto {

bool HandshakeReplyMessage::SerializeBody(co::BinWriter& writer) const {
  uint32_t success_val = success_ ? 1 : 0;
  if (!writer.WriteUint32(success_val)) {
    return false;
  }
  if (!writer.WriteUint32(client_ping_interval_sec_)) {
    return false;
  }
  return true;
}

bool HandshakeReplyMessage::UnserializeBody(co::BinReader& reader) {
  uint32_t success_val;
  uint32_t client_ping_interval_sec;
  if (!reader.ReadUint32(success_val)) {
    return false;
  }
  if (!reader.ReadUint32(client_ping_interval_sec)) {
    return false;
  }
  if (reader.BytesLeft() != 0) {
    // We have an unexpected trail.
    return false;
  }
  switch (success_val) {
  case 1:
    success_ = true;
    break;
  case 0:
    success_ = false;
    break;
  default:
    return false;
  }
  client_ping_interval_sec_ = client_ping_interval_sec;
  return true;
}

bool HandshakeReplyMessage::CompareBody(const ProtoMessage& r) const {
  return
    this->success_ == static_cast<const HandshakeReplyMessage&>(r).success_ &&
    this->client_ping_interval_sec_ == static_cast<const HandshakeReplyMessage&>(r).client_ping_interval_sec_;
}

}}