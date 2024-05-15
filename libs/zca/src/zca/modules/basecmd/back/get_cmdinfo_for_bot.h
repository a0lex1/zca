#pragma once

#include "zca/modules/basecmd/back/basecmd_bot_module_data.h"

namespace modules {
namespace basecmd {
namespace back {

// Threadsafe (|bot| must be kept of course), return nullptr if bot module data isn't yet set
co::InterlockedHolder<CmdInfo>* GetCmdInfoHolderForBot(
  cc::ICcBot& bot,
  engine::Module& basecmd_module)
{
  BasecmdBotModuleData* bbmd = basecmd_module.GetLocalApiAs<core::back::BackendLocalApi>()
    .GetBotModuleDataAs<BasecmdBotModuleData>(bot);

  if (bbmd == nullptr) {
    // If the bot isn't yet handshaked, bot module data is nullptr.
    return nullptr;
  }
  return &bbmd->GetCmdInfoInterlockedHolder();
}

}}}


