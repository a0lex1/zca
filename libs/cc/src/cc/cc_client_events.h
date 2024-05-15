#pragma once

#include "co/common.h"

namespace cc {

class CcClientEvents {
public:
  virtual ~CcClientEvents() = default;

  // Fires when handshake write succeeded
  virtual void OnClientHandshakeWritten() {}
  virtual void OnClientHandshakeReplyReceived() {}
};


}

