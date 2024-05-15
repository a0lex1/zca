#include "cc/find_bot_in_list.h"

using namespace std;

namespace cc {

Shptr<ICcBot> FindBotInList(ICcBotList& bot_list, const BotId& bot_id) {
  // inside bot list fiber
  for (auto& bot : bot_list) {
    ICcBotHandshakeData* hsdata(bot->GetReadonlyData().GetHandshakeData());
    if (hsdata == nullptr) {
      continue; // not yet handshaked in cc
    }
    if (hsdata->GetBotId() == bot_id) {
      return bot;
    }
  }
  return nullptr;
}







}




