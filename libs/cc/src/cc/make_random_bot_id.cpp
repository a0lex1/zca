#include "cc/make_random_bot_id.h"

namespace cc {

BotId MakeRandomBotId(co::RandGen& rng) {
  return BotId::FromUints(rng.RandInt<uint64_t>(), rng.RandInt<uint64_t>());
}

}
