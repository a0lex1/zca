#pragma once

#include "zca/engine/sdk/dispatch_cmd_task.h"

#include "netshell/netshell_error.h"

namespace modules {
namespace basecmd {
namespace front {

class BotlistCmdTask: public engine::DispatchCmdTask,
  public co::enable_shared_from_this<BotlistCmdTask>
{
public:
  virtual ~BotlistCmdTask() = default;

  BotlistCmdTask(Shptr<Strand> strand);

private:
  void BeginIo(RefTracker) override;
  void StopUnsafeExtra() override;

  void HandleExecBackCmdline(netshell::NetshellError, RefTracker);

private:
  bool f_no_csv_header_{ false };
  bool f_csv_{ false };
  netshell::NsCmdResult back_result_;
};

}}}


