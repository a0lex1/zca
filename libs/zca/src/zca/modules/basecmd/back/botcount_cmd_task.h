#pragma once

#include "zca/modules/basecmd/back/basecmd_backend_module.h"

#include "zca/engine/sdk/dispatch_cmd_task.h"

namespace modules {
namespace basecmd {
namespace back {

class BotcountCmdTask
  :
  public engine::DispatchCmdTask,
  public co::enable_shared_from_this<BotcountCmdTask>
{
public:
  virtual ~BotcountCmdTask() = default;

  BotcountCmdTask(Shptr<Strand> strand);

private:
  void BeginIo(co::RefTracker) override;
  void HandleExecuteBotCallback(cc::ICcBotList& bot_list, co::RefTracker);
  void StopUnsafeExtra() override;

private:
  EmptyHandler user_handler_;
  netshell::NsCmdResult* user_cmd_result_{ nullptr };
};

}}}


