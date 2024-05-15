#pragma once

#include "cc/bot_handshake_data.h"
#include "co/common.h"

namespace cc {

struct BotReadonlyData : public ICcBotReadonlyData {
  virtual ~BotReadonlyData() = default;

  // [ICcBotReadonlyData impl]
  // Can return nullptr if handshake isn't done yet.
  ICcBotHandshakeData* GetHandshakeData() override {
    return hshake_data_.get();
  }

  boost::posix_time::ptime GetLastPingTime() override {
    return last_ping_time_;
  }

  co::net::Endpoint GetRemoteAddress() override {
    return remote_addr_;
  }

  Uptr<BotHandshakeData> hshake_data_;
  boost::posix_time::ptime last_ping_time_;
  co::net::Endpoint remote_addr_;
};


}


