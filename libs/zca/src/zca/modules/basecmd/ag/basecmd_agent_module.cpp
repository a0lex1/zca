#include "zca/modules/basecmd/ag/basecmd_agent_module.h"
#include "zca/modules/basecmd/ag/shell_cmd_task.h"

#include "co/xlog/xlog.h"

using namespace co;

namespace modules {
namespace basecmd {
namespace ag {

BasecmdAgentModule::BasecmdAgentModule()
{
  // Commands are executed by CmdListExecutionTask on strand.
  SET_DEBUG_TAG(*this, "BasecmdAgentModule");

  disp_table_.AddCommand("suicide", co::bind(&BasecmdAgentModule::DispatchSuicideCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("shell", co::bind(&BasecmdAgentModule::DispatchShellCmd, this, _1, _2, _3, _4));
}

void BasecmdAgentModule::OnSessionCreated(engine::EngineSessionApi& sess_api,
                                          Uptr<engine::SessionModuleData>& module_data) {
  // allocate data for module in admin session
}

void BasecmdAgentModule::DispatchShellCmd(engine::DispatchContext& disp_context,
                                          engine::DispatchCmdData&,
                                          Shptr<Strand> strand,
                                          Shptr<engine::DispatchCmdTask>& new_task)
{
  //auto& ioctx_sess = disp_context.GetSessionApi().GetSessionIoContext();
  new_task = make_shared<ShellCmdTask>(strand);

  SET_DEBUG_TAG(*new_task, "basecmd.ShellCmdTask");
}

void BasecmdAgentModule::DispatchSuicideCmd(engine::DispatchContext& dispatch_ctx,
                                            engine::DispatchCmdData& dispatch_data,
                                            Shptr<Strand> strand,
                                            Shptr<engine::DispatchCmdTask>& new_task) {

  syslog(_WARN) << "************  SUICIDING  ************\n";

  GetGlobalApiAs<core::ag::AgentGlobalApi>().GetCcClient().StopThreadsafe();

  // Result will not be delivered
}

}}}
