#include "cc/cc_bot_list.h"

namespace cc {

// Call only from bot list fiber!
Shptr<ICcBot> FindBotInList(ICcBotList& bot_list, const BotId& bot_id);

}

