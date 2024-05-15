#pragma once

#include "proto/proto_message.h"
#include "cc/cc_proto/codes.h"

namespace cc {
namespace cc_proto {

class MessageFactory;
class HandshakeReplyMessage: public ProtoMessage {
  static const ProtoMessageCode kCode = codes::kHandshakeReply;
public:
  virtual ~HandshakeReplyMessage() = default;

  HandshakeReplyMessage(
    bool success,
    uint32_t client_ping_interval_sec)
    : ProtoMessage(kCode)
  {
    success_ = success;
    client_ping_interval_sec_ = client_ping_interval_sec;
  }

  bool GetSuccess() const { return success_; }
  uint32_t GetClientPingIntervalMsec() const { return client_ping_interval_sec_; }

  co::DebugTagOwner _DbgTagForMsgCode(ProtoMessageCode code) const override {
    return MakeDebugTag("HandshakeReplyMessage");
  }


private:
  bool success_;
  uint32_t client_ping_interval_sec_;

private:
  bool CompareBody(const ProtoMessage&) const override;
  bool SerializeBody(co::BinWriter& writer) const override;
  bool UnserializeBody(co::BinReader& reader) override;

  friend class MessageFactory;
  HandshakeReplyMessage(): ProtoMessage(kCode) {}
};

}}

