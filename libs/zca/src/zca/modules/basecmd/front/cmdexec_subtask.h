#pragma once

#include "zca/core/front/admin_session_custom_api.h"

#include "zca/engine/sdk/engine_session.h"
#include "zca/engine/sdk/cmd_executor.h"

#include "netshell/ns_para_command_executor.h"

#include "co/async/task_manager.h"

namespace modules {
namespace basecmd {
namespace front {

class CmdexecSubtask
  :
  public co::async::TaskImpl,
  public co::enable_shared_from_this<CmdexecSubtask>
{
public:
  virtual ~CmdexecSubtask() = default;

  CmdexecSubtask(
    Shptr<Strand> strand,
    core::front::AdminSessionCustomApi& custom_api,
    engine::EngineSessionApi& sess_api,
    engine::CmdExecutor& cmd_executor,
    const std::string& back_cmdline,
    const std::string& fpost_cmdline);

  const netshell::NsCmdResult& GetFinalResult() const;

private:
  void BeginIo(co::RefTracker rt) override;
  void StopUnsafeExtra() override;

  void HandleExecute(netshell::NetshellError, RefTracker);
  void HandleFPostExecute();


private:
  core::front::AdminSessionCustomApi& custom_api_;
  engine::EngineSessionApi& sess_api_;
  engine::CmdExecutor& cmd_executor_;
  std::string back_cmdline_;
  std::string fpost_cmdline_;
  netshell::NsCmdResult cmd_result_;
  netshell::NsCmdResult fpost_result_;
};

}}}

