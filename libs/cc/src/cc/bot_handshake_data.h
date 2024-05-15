#pragma once

#include "cc/cc_proto/handshake_message.h"

#include "cc/cc_bot_data.h"

namespace cc {

class BotHandshakeData : public ICcBotHandshakeData {
public:
  virtual ~BotHandshakeData() = default;

  BotHandshakeData(const cc_proto::HandshakeMessage& hshake_msg)
    : hshake_msg_(hshake_msg)
  {
  }

  cc_proto::HandshakeMessage& GetHandshakeMessage() {
    return hshake_msg_;
  }

  // [ICcBotHandshakeData impl]
  const BotId& GetBotId() const override {
    return hshake_msg_.GetBotId();
  }
  BotVersion GetBotVersion() const override {
    return hshake_msg_.GetBotVersion();
  }
  const std::string& GetOpaqueData() const override {
    return hshake_msg_.GetOpaqueData();
  }

private:
  cc_proto::HandshakeMessage hshake_msg_;
};

}

