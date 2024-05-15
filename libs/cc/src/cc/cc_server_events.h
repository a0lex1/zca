#pragma once

#include "cc/cc_bot.h"
#include "cc/cc_bot_data.h"

#include "co/common.h"

namespace cc {

class CcServerEvents {
public:
  virtual ~CcServerEvents() = default;

  // Funcs run in bot fiber

  /**/
  virtual void OnBotHandshakeComplete(Shptr<ICcBot> bot) {}

  /**/
  virtual void OnBotRemovedFromList(Shptr<ICcBot> bot) {}
};

}





