#include "zca/modules/basecmd/back/kill_cmd_task.h"
#include "zca/modules/basecmd/back/basecmd_backend_module.h"
#include "zca/netshell_status_descriptor_table.h"

#include "co/async/wrap_post.h"
#include "co/base/cmdline/parse_cmdline_to_argv.h"
#include "co/xlog/xlog.h"

#include <sstream>

using namespace std;
using namespace co;
using namespace co::cmdline;
using namespace netshell;
using namespace boost::posix_time;

#define llog() Log(_DBG) << "KillCmdTask (base)" << \
    SPTR(static_cast<co::async::Task*>(this)) << " "

namespace modules {
namespace basecmd {
namespace back {

DEFINE_XLOGGER_SINK("killcmdtask", gZcaKillCmdTaskSink);
#define XLOG_CURRENT_SINK gZcaKillCmdTaskSink

KillCmdTask::KillCmdTask(Shptr<Strand> strand)
  : DispatchCmdTask(strand)
{
}

void KillCmdTask::StopUnsafeExtra() {
  // No way to stop the task. It should complete on it's own.
  llog() << "StopUnsafe\n";
}

void KillCmdTask::BeginIo(RefTracker rt) {
  llog() << "BeginIo\n";

  rt.SetReferencedObject(shared_from_this());

  // Finding bot by ID requires the execution of botlist access callback; continue from there
  GetModule().GetGlobalApiAs<core::back::BackendGlobalApi>()
    .GetCcServer()
    .ExecuteBotListAccessCallback(
      co::bind(&KillCmdTask::BotListAccessCallback,
        shared_from_this(), _1, rt));
}


void KillCmdTask::BotListAccessCallback(cc::ICcBotList& bot_list,
                                        RefTracker rt) {
  StringVector argv;
  ParseCmdlineToArgv(GetNsCommand().c_str(), argv);

  DCHECK(argv[0] == "kill");

  if (argv.size() != 2) {
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
      .WithMessageBody(string_printf("Need 1 args, not %d", argv.size()-1)));
    return;
  }

  cc::BotId bot_id;
  if (!bot_id.FromStringRepr(argv[1])) {
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
      .WithMessageBody(string_printf("invalid bid - %s", bot_id.ToStringRepr().c_str())));
    return;
  }

  Shptr<cc::ICcBot> bot = cc::FindBotInList(bot_list, bot_id);
  if (!bot) {
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
      .WithMessageBody(string_printf("bid not exist - %s", bot_id.ToStringRepr().c_str())));
    return;
  }

  // Now kill the bot
  //
  bot->Kill();

  // Set the result 
  //
  SetResult(NsCmdResult(kNsCmdExecuted, 0, NsResultType::kMessage)
    .WithMessageBody("bot killed"));
}



}}}
