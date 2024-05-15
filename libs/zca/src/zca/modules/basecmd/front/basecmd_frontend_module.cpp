#include "zca/modules/basecmd/front/basecmd_frontend_module.h"
#include "zca/modules/basecmd/front/botlist_cmd_task.h"
#include "zca/modules/basecmd/front/cmdexec_cmd_task.h"
#include "zca/modules/basecmd/front/kill_cmd_task.h"

#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace netshell;
using namespace engine;

namespace modules {
namespace basecmd {
namespace front {

BasecmdFrontendModule::BasecmdFrontendModule()
{
  SET_DEBUG_TAG(*this, "BasecmdFrontendModule");
  disp_table_.AddCommand("bot-list", co::bind(&BasecmdFrontendModule::DispatchBotlistCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("cmd-exec", co::bind(&BasecmdFrontendModule::DispatchCmdexecCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("kill", co::bind(&BasecmdFrontendModule::DispatchKillCmd, this, _1, _2, _3, _4));
  //disp_table_.AddCommand("sync", co::bind(&BasecmdFrontendModule::DispatchExitCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("exit", co::bind(&BasecmdFrontendModule::DispatchExitCmd, this, _1, _2, _3, _4));
}

void BasecmdFrontendModule::OnSessionCreated(EngineSessionApi&,
                                             Uptr<SessionModuleData>& ptr) {
  // Allocate data for module in admin session
}


// Command dispatchers

void BasecmdFrontendModule::DispatchBotlistCmd(DispatchContext& disp_context,
  DispatchCmdData&,
  Shptr<Strand> strand,
  Shptr<DispatchCmdTask>& new_task)
{
  new_task = make_shared<BotlistCmdTask>(
    make_shared<Strand>(disp_context.GetSessionApi().GetSessionIoContext()));
}

void BasecmdFrontendModule::DispatchCmdexecCmd(DispatchContext& disp_context,
  DispatchCmdData&,
  Shptr<Strand> strand,
  Shptr<DispatchCmdTask>& new_task)
{
  new_task = make_shared<CmdexecCmdTask>(strand);

  SET_DEBUG_TAG(*new_task, "basecmd.CmdexecCmdTask");
}

void BasecmdFrontendModule::DispatchKillCmd(DispatchContext& disp_context,
  DispatchCmdData&,
  Shptr<Strand> strand,
  Shptr<DispatchCmdTask>& new_task)
{
  new_task = make_shared<KillCmdTask>(strand);
}

void BasecmdFrontendModule::DispatchExitCmd(DispatchContext& disp_context,
  DispatchCmdData&,
  Shptr<Strand> strand,
  Shptr<DispatchCmdTask>&)
{
  auto& sessapi(disp_context.GetSessionApi());
  syslog(_DBG) << "Session " << sessapi.GetSessionInfo().GetSessionId() << " sent `exit` cmd, closing it\n";
  sessapi.KillSession();
}

}}}

