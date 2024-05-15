#pragma once

#include "zca/engine/sdk/dispatch_cmd_task.h"

#include "cc/find_bot_in_list.h"

#include "co/xlog/define_logger_sink.h"

namespace modules {
namespace basecmd {
namespace back {

DECLARE_XLOGGER_SINK("killcmdtask", gZcaKillCmdTaskSink);

class KillCmdTask : public engine::DispatchCmdTask,
  public co::enable_shared_from_this<KillCmdTask>
{
public:
  virtual ~KillCmdTask() = default;

  KillCmdTask(Shptr<Strand> strand);

private:
  void BeginIo(RefTracker) override;
  void StopUnsafeExtra() override;

  void BotListAccessCallback(cc::ICcBotList&, RefTracker);

private:

};

}}}


