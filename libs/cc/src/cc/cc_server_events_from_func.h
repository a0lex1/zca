#pragma once

#include "cc/cc_bot_list.h"
#include "cc/cc_server_events.h"

namespace cc {

class CcServerEventsFromFunc : public CcServerEvents {
public:
  virtual ~CcServerEventsFromFunc() = default;

  typedef Func<void(Shptr<ICcBot> bot)> OnBotHandshakeCompleteFunc;
  typedef Func<void(Shptr<ICcBot> bot)> OnBotRemovedFromListFunc;

  using CcServerEvents::OnBotHandshakeComplete;
  using CcServerEvents::OnBotRemovedFromList;

  void SetOnBotHandshakeComplete(OnBotHandshakeCompleteFunc func) { on_bot_hshake_complete_ = func; }
  void SetOnBotRemoved(OnBotRemovedFromListFunc func) { on_bot_removed_ = func; }

private:
  // CcServerEvents impl.
  void OnBotHandshakeComplete(Shptr<ICcBot> bot) override {
    if (on_bot_hshake_complete_) {
      on_bot_hshake_complete_(bot);
    }
  }
  void OnBotRemovedFromList(Shptr<ICcBot> bot) override {
    if (on_bot_removed_) {
      on_bot_removed_(bot);
    }
  }

private:
  OnBotHandshakeCompleteFunc on_bot_hshake_complete_;
  OnBotRemovedFromListFunc on_bot_removed_;
};

}

