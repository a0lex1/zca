#pragma once

#include "cc/bot_version.h"
#include "cc/bot_id.h"
#include "co/net/endpoint.h"
#include "co/common.h"

#include <boost/date_time/posix_time/ptime.hpp>

namespace cc {

class ICcBotHandshakeData {
public:
  virtual ~ICcBotHandshakeData() = default;

  virtual const BotId& GetBotId() const = 0;
  virtual BotVersion GetBotVersion() const = 0;
  virtual const std::string& GetOpaqueData() const = 0;
};

class ICcBotReadonlyData {
public:
  virtual ~ICcBotReadonlyData() = default;

  // can return nullptr if handshake hasn't been done yet
  virtual ICcBotHandshakeData* GetHandshakeData() = 0;

  // Returns 0 if no pings done yet
  virtual boost::posix_time::ptime GetLastPingTime() = 0;

  virtual co::net::Endpoint GetRemoteAddress() = 0;
};

}

