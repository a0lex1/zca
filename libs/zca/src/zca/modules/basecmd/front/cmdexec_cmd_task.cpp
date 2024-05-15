#include "zca/modules/basecmd/front/cmdexec_cmd_task.h"
#include "zca/modules/basecmd/front/cmdexec_subtask.h"

#include "zca/core/front/admin_session_custom_api.h"
#include "zca/core/res_with_msg_body.h"

#include "zca/netshell_status_descriptor_table.h"

#include "co/async/wrap_post.h"
#include "co/base/cmdline/cmdline_section_split.h"
#include "co/base/cmdline/parse_cmdline_to_argv.h"
#include "co/base/cmdline/join_cmd_line.h"
#include "co/base/cmdline/getopt_cpp.h"
#include "co/base/base64.h"

using namespace std;
using namespace co;
using namespace co::cmdline;
using namespace co::async;
using namespace netshell;

namespace modules {
namespace basecmd {
namespace front {

// cmdexec [-tT] [-w] echo-args 1 2 3
CmdexecCmdTask::CmdexecCmdTask(Shptr<Strand> strand)
  : DispatchCmdTask(strand)
{
}

void CmdexecCmdTask::BeginIo(RefTracker rt) {
  rt.SetReferencedObject(shared_from_this());

  //StringVector side_sections_;
  CmdlineSectionSplit(GetNsCommand(), side_sections_, " | ");
  if (side_sections_.size() != 1 && side_sections_.size() != 2 && side_sections_.size() != 3) {
    SetResult(core::ResWithMsgBody(-1,
      string_printf("expected 1, 2 or 3 sections, not %d", side_sections_.size())));
    return;
  }

  StringVector args;
  if (!cmdline::ParseCmdlineToArgv(side_sections_[0].c_str(), args)) {
    SetResult(core::ResWithMsgBody(-1, "Cannot parse cmdline"));
    return;
  }

  bool reserved_x = false, reserved_y = false;
  Getopt go;
  int getopt_ret;
  while ((getopt_ret = go.Execute(args, "nxy")) != -1) {
    switch (getopt_ret) {
    case 'n':
      do_wait_ = false;
      break;
    case 'x':
      reserved_x = true;
      break;
    case 'y':
      reserved_y = true;
      break;
    case '?':
      SetResult(core::ResWithMsgBody(-1, "Ambiguous parameter"));
      return;
    default:
      NOTREACHED();
    }
  }
  if (args.size() < go.OptInd() + 2) {
    SetResult(core::ResWithMsgBody(-1, "Need bid + at least one element for cmdline"));
    return;
  }
  string& str_bid = args[go.OptInd()];
  if (!bot_id_.FromStringRepr(str_bid)) {
    SetResult(core::ResWithMsgBody(-1, string_printf("invalid bid - %s", str_bid.c_str())));
    return;
  }
  bot_cmdline_ = JoinCmdLine(args, go.OptInd() + 1);

  auto& para_executor(GetCustomApiAs<core::front::AdminSessionCustomApi>()->GetBackshellParaExecutor());

  string cmdline = "bot-list -c -n -a bid -a salt -i bid%" + bot_id_.ToStringRepr();

  para_executor.ExecuteCommand(cmdline, getsalt_cmdresult_,
    wrap_post(GetFiberStrand(), co::bind(&CmdexecCmdTask::HandleGetSalt,
      shared_from_this(), _1, rt)));
}


void CmdexecCmdTask::HandleGetSalt(netshell::NetshellError ns_err, RefTracker rt) {
  if (ns_err) {
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
      .WithMessageBody("Netshell error "+ns_err.MakeErrorMessage()));
    return;
  }
  if (getsalt_cmdresult_.csv_rows.size() == 0) {
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
      .WithMessageBody("Bot not found"));
    return;
  }
  if (getsalt_cmdresult_.status_code != kNsCmdExecuted
    || getsalt_cmdresult_.result_type != NsResultType::kCsv
    || getsalt_cmdresult_.csv_rows.size() != 1)
  {
    NsCmdResultTextualizer texer(gZcaNsStatusDescriptorTable, getsalt_cmdresult_);
    string first_line;
    texer.TextualizeFirstLine(first_line);
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
      .WithMessageBody("Malformed getsalt result - " + first_line));
    return;
  }
  const string& salt(getsalt_cmdresult_.csv_rows[0][1]);

  // concat everything as a string
  string fruitsalad = string_to_upper(bot_id_.ToStringRepr())
    + salt
    + bot_cmdline_;

  auto& sig_creator(GetCustomApiAs<core::front::AdminSessionCustomApi>()->GetSignatureCreator());
  std::string signature;
  sig_creator.CreateSignature(fruitsalad.c_str(), fruitsalad.length(), signature);

  string back_cmdline;
  back_cmdline = "cmd-exec "+bot_id_.ToStringRepr()+" "+bot_cmdline_+" ^^ "+co::encode64(signature, false);
  if (side_sections_.size() > 1) {
    back_cmdline += " | " + side_sections_[1];
  }
  string fpost_cmdline;
  if (side_sections_.size() > 2) {
    DCHECK(side_sections_.size() == 3);
    fpost_cmdline = side_sections_[2];
  }

  // The following execution is inside subtask, because we need both wait/nowait modes
  Shptr<CmdexecSubtask> subtask = make_shared<CmdexecSubtask>(
    GetFiberStrandShptr(),
    static_cast<core::front::AdminSessionCustomApi&>(*GetCustomApi()),
    GetDispatchContext().GetSessionApi(),
    GetDispatchContext().GetCmdExecutor(),
    back_cmdline,
    fpost_cmdline);

  RefTracker sub_rt;
  if (do_wait_) {
    sub_rt = RefTracker(CUR_LOC(), [=]() {
      // CONNECTED to |rt|
      SetResult(subtask->GetFinalResult());
      },
      rt);
  }
  else {
    sub_rt = RefTracker(CUR_LOC(), rt.GetContextHandle(), []() {
      // NOT connected to |rt|
      // background task io ended
      });
    SetResult(core::ResWithMsgBody(0, "Executed in background"));
  }
  GetDispatchContext().GetTaskManager().ExecuteTask(subtask, sub_rt);
}


void CmdexecCmdTask::StopUnsafeExtra() {

}


}}}


