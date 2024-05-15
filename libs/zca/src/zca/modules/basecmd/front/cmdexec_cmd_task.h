#pragma once

#include "zca/engine/sdk/dispatch_cmd_task.h"

#include "netshell/netshell_error.h"

#include "zca/core/bot_filter.h"
#include "zca/core/field_filter.h"

#include "cc/bot_id.h"

namespace modules {
namespace basecmd {
namespace front {

class CmdexecCmdTask : public engine::DispatchCmdTask,
  public co::enable_shared_from_this<CmdexecCmdTask>
{
public:
  virtual ~CmdexecCmdTask() = default;

  CmdexecCmdTask(Shptr<Strand> strand);

private:
  void BeginIo(co::RefTracker) override;

  void HandleGetSalt(netshell::NetshellError, RefTracker);

  void StopUnsafeExtra() override;

private:
  StringVector side_sections_;
  bool do_wait_{ true };
  cc::BotId bot_id_;
  std::string bot_cmdline_;
  netshell::NsCmdResult getsalt_cmdresult_;
  netshell::NsCmdResult cmd_result_;
  netshell::NsCmdResult post_result_;
};

}}}


