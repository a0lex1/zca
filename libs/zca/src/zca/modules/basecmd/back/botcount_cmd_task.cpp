#include "zca/modules/basecmd/back/botcount_cmd_task.h"
#include "zca/core/back/backend_api.h"

#include "zca/netshell_status_descriptor_table.h"

#include "co/base/cmdline/parse_cmdline_to_argv.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::cmdline;
using namespace netshell;

namespace modules {
namespace basecmd {
namespace back {

BotcountCmdTask::BotcountCmdTask(Shptr<Strand> strand) 
  : DispatchCmdTask(strand)
{

}

void BotcountCmdTask::BeginIo(RefTracker rt) {
  rt.SetReferencedObject(shared_from_this());

  auto& cmd = GetNsCommand();
  StringVector argv;
  if (ParseCmdlineToArgv(cmd.c_str(), argv)) {
    DCHECK(argv[0] == "bot-count");
    if (argv.size() == 1) {
      GetModule().GetGlobalApiAs<core::back::BackendGlobalApi>()
        .GetCcServer().ExecuteBotListAccessCallback(
          co::bind(&BotcountCmdTask::HandleExecuteBotCallback, shared_from_this(), _1, rt));
    }
  }
}

void BotcountCmdTask::HandleExecuteBotCallback(cc::ICcBotList& bot_list, co::RefTracker rt) {
  auto bot_count(bot_list.GetCount());
  auto bot_count_str(string_from_uint64(bot_count));
  SetResult(NsCmdResult(
    kNsCmdExecuted, 0, NsResultType::kText
  ).WithTextBody(bot_count_str));

  // Disposal of |rt| will complete the task
}

void BotcountCmdTask::StopUnsafeExtra() {

}

}}}

