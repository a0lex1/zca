#pragma once

#include "cc/cc_client_events.h"

namespace cc {

class CcClientEventsFromFunc: public CcClientEvents {
public:
  virtual ~CcClientEventsFromFunc() = default;

  typedef EmptyHandler EventHandlerFunc;

  void SetOnHandshakeWritten(EventHandlerFunc on_handshake) { on_handshake_ = on_handshake; }
  void SetOnHandshakeReplyReceived(EventHandlerFunc on_handshake_rr) { on_handshake_rr_ = on_handshake_rr; }

private:
  // CcClientEvents impl.
  virtual void OnClientHandshakeWritten() override {
    if (on_handshake_) {
      on_handshake_();
    }
  }

  virtual void OnClientHandshakeReplyReceived() override {
    if (on_handshake_rr_) {
      on_handshake_rr_();
    }
  }

private:
  EventHandlerFunc on_handshake_;
  EventHandlerFunc on_handshake_rr_;
};

}

