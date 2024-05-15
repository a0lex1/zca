#pragma once

#include "zca/engine/sdk/dispatch_cmd_task.h"

#include "netshell/netshell_error.h"

namespace modules {
namespace basecmd {
namespace front {

class KillCmdTask : public engine::DispatchCmdTask,
  public co::enable_shared_from_this<KillCmdTask>
{
public:
  virtual ~KillCmdTask() = default;

  KillCmdTask(Shptr<Strand> strand);

private:
  void BeginIo(RefTracker) override;
  void StopUnsafeExtra() override;

  void HandleBackExecute(netshell::NetshellError, RefTracker);

private:
  netshell::NsCmdResult back_result_;
};

}}}


