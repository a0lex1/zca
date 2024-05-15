#pragma once

#include "cc/cc_server_events.h"
#include "cc/cc_client_events.h"

#include "co/base/filter_chain.h"

namespace core {

class CcServerEventsLink : public co::FilterChainLink<cc::CcServerEvents> {
public:
  virtual ~CcServerEventsLink() = default;

  void OnBotHandshakeComplete(Shptr<cc::ICcBot> bot) override {
    if (GetNext()) {
      GetNext()->OnBotHandshakeComplete(bot);
    }
  }
  void OnBotRemovedFromList(Shptr<cc::ICcBot> bot) override {
    if (GetNext()) {
      GetNext()->OnBotRemovedFromList(bot);
    }
  }
};

class CcClientEventsLink : public co::FilterChainLink<cc::CcClientEvents> {
public:
  virtual ~CcClientEventsLink() = default;

  void OnClientHandshakeWritten() override {
    if (GetNext()) {
      GetNext()->OnClientHandshakeWritten();
    }
  }
  void OnClientHandshakeReplyReceived() override {
    if (GetNext()) {
      GetNext()->OnClientHandshakeReplyReceived();
    }
  }
};

using CcServerEventsFilterChainHead = co::FilterChainHead<CcServerEventsLink>;
using CcClientEventsFilterChainHead = co::FilterChainHead<CcClientEventsLink>;

}

