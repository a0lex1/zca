#pragma once

#include "cc/bot_version.h"
#include "cc/bot_id.h"
#include "cc/cc_proto/codes.h"

#include "proto/proto_message.h"

namespace cc {
namespace cc_proto {

class MessageFactory;
class HandshakeMessage: public ProtoMessage {
  static const ProtoMessageCode kCode = codes::kHandshake;
public:
  virtual ~HandshakeMessage() = default;
  
  // |opaque_data| is copied
  HandshakeMessage(const BotId& bot_id, BotVersion bot_ver,
    const std::string& opaque_data)
    :
    ProtoMessage(kCode),
    bot_id_(bot_id),
    bot_ver_(bot_ver),
    opaque_data_(opaque_data)
  {
  }

  const BotId& GetBotId() const {
    return bot_id_;
  }

  uint32_t GetBotVersion() const {
    return bot_ver_;
  }

  const std::string& GetOpaqueData() const {
    return opaque_data_;
  }

  co::DebugTagOwner _DbgTagForMsgCode(ProtoMessageCode code) const override {
    return MakeDebugTag("cc_proto::HandshakeMessage");
  }

private:
  BotId bot_id_;
  BotVersion bot_ver_;
  std::string opaque_data_;

private:
  bool CompareBody(const ProtoMessage&) const override;
  bool SerializeBody(co::BinWriter& writer) const override;
  bool UnserializeBody(co::BinReader& reader) override;

  friend class MessageFactory;

  HandshakeMessage(): ProtoMessage(kCode), bot_ver_(0) {}
};

}}

