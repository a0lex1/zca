#include "zca/modules/basecmd/back/cmdexec_cmd_task.h"
#include "zca/core/cmd_opaque_data.h"
#include "zca/core/res_with_msg_body.h"
#include "zca/netshell_status_descriptor_table.h"

#include "cc/find_bot_in_list.h"

#include "netshell/unserializer.h"

#include "co/async/wrap_post.h"
#include "co/base/cmdline/cmdline_section_split.h"
#include "co/base/cmdline/parse_cmdline_to_argv.h"
#include "co/base/cmdline/getopt_cpp.h"
#include "co/base/cmdline/join_cmd_line.h"
#include "co/xlog/xlog.h"

#include <sstream>

using namespace std;
using namespace co;
using namespace co::cmdline;
using namespace netshell;
using namespace boost::posix_time;

#define llog() Log(_DBG) << "CmdexecCmdTask (base)" << \
    SPTR(static_cast<co::async::Task*>(this)) << " "

namespace modules {
namespace basecmd {
namespace back {

DEFINE_XLOGGER_SINK("cmdexeccmdtask", gZcaCmdexecCmdTaskSink);
#define XLOG_CURRENT_SINK gZcaCmdexecCmdTaskSink

CmdexecCmdTask::CmdexecCmdTask(Shptr<Strand> strand)
  : DispatchCmdTask(strand)
{
}

void CmdexecCmdTask::StopUnsafeExtra() {
  // No way to stop the task. It should complete on it's own.
  llog() << "StopUnsafe\n";
}

void CmdexecCmdTask::BeginIo(RefTracker rt) {
  llog() << "BeginIo\n";

  rt.SetReferencedObject(shared_from_this());

  // Parse first, then if successfully parsed, execute bot list access callback
  CmdlineSectionSplit(GetNsCommand(), side_sections_, " | ");

  if (side_sections_.size() != 1 && side_sections_.size() != 2) {
    SetResult(core::ResWithMsgBody(-1, string_printf("expected 1 or 2 side (|) sections, not %d", side_sections_.size())));
    return;
  }

  CmdlineSectionSplit(side_sections_[0], sig_sections_, "^^");
  if (sig_sections_.size() != 1 && sig_sections_.size() != 2) {
    SetResult(core::ResWithMsgBody(-1, string_printf("expected 1 or 2 ^^ sections, not %d", sig_sections_.size())));
    return;
  }

  StringVector args;
  if (!cmdline::ParseCmdlineToArgv(sig_sections_[0].c_str(), args)) {
    SetResult(core::ResWithMsgBody(-1, "Cannot parse cmdline"));
    return;
  }

  bool reserved_x = false, reserved_y = false;
  Getopt go;
  int getopt_ret;
  while ((getopt_ret = go.Execute(args, "xy")) != -1) {
    switch (getopt_ret) {
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
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
      .WithMessageBody(string_printf("invalid bid - %s", str_bid.c_str())));
    return;
  }

  bot_cmdline_ = JoinCmdLine(args, go.OptInd() + 1);

  // Finding bot by ID requires the execution of botlist access callback; continue from there
  GetModule().GetGlobalApiAs<core::back::BackendGlobalApi>()
    .GetCcServer()
    .ExecuteBotListAccessCallback(
      co::bind(&CmdexecCmdTask::BotListAccessCallback,
        shared_from_this(), _1, rt));
}


void CmdexecCmdTask::BotListAccessCallback(cc::ICcBotList& bot_list,
                                           RefTracker rt) {
  llog() << "BotListAccessCallback\n";

  // Find bot by id
  bot_ = cc::FindBotInList(bot_list, bot_id_);
  if (!bot_) {
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
               .WithMessageBody(string_printf("bid not exist - %s", bot_id_.ToStringRepr().c_str())));
    return;
  }

  bmdata_ = GetModule().GetLocalApiAs<core::back::BackendLocalApi>()
    .GetBotModuleDataAs<BasecmdBotModuleData>(*bot_.get());
  if (bmdata_ == nullptr) {
    // Bot hasn't handshaken yet. TODO: check if race cond !!!!!!!!!!!
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
      .WithMessageBody(string_printf("bot not handshaked yet - %s", bot_id_.ToStringRepr().c_str())));
    return;
  }

  // <<< LOCK >>>

  if (!bmdata_->AcquireCmdLock()) {
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
      .WithMessageBody("another command in progress"));
    return;
  }
  auto& cmdinfo_holder = bmdata_->GetCmdInfoInterlockedHolder();

  // change cmdinfo state
  
  cmdinfo_holder.SafelyChangeData([&](CmdInfo& cmd_info) {
    cmd_info.state = CmdState::kNewCommand;
    cmd_info.bot_command = bot_cmdline_;
    cmd_info.bot_result.Clear();
    cmd_info.post_command.clear();
    cmd_info.post_result.Clear();
    cmd_info.net_error.clear();
    cmd_info.netshell_error = NetshellError::NoError();
    });


  DCHECK(sig_sections_.size() == 1 || sig_sections_.size() == 2);
  string cmd_opaque_buf;
  core::CmdOpaqueData opaque_data;
  opaque_data.cmdline = bot_cmdline_;
  if (sig_sections_.size() == 2) {
    opaque_data.signature = sig_sections_[1];
  }
  co::BinWriter writer(cmd_opaque_buf);
  opaque_data.Serialize(writer);

  // This will send |bot_section| command line to bot.
  bot_->ExecuteSequencedCommand(
    make_unique<string>(cmd_opaque_buf),
    cmd_result_opaque_buf_,
    co::async::wrap_post(GetFiberStrand(), co::bind(
      &CmdexecCmdTask::HandleExecuteSequencedCommand,
      shared_from_this(), _1, rt)));
}


void CmdexecCmdTask::HandleExecuteSequencedCommand(Errcode err, RefTracker rt) {
  llog() << "HandleExecuteSequencedCommand (err = " << err << ")\n";

  auto& cmdinfo_holder = bmdata_->GetCmdInfoInterlockedHolder();

  // handle error cases
  if (err) {
    cmdinfo_holder.SafelyChangeData([&](CmdInfo& cmd_info) {
      cmd_info.state = CmdState::kNetworkError;
      cmd_info.net_error = err;
      });
    stringstream err_ss;
    err_ss << err;
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
      .WithMessageBody(string_printf("network err %s", err_ss.str().c_str())));
    return;
  }
  // unserialize the bot result (we will need to reflect changes in cmd_info.bot_result)
  // === other end zca/core/ag/agent_core.cpp ===
  NsCmdResultUnserializer unserer(gZcaNsStatusDescriptorTable, unsered_result_);
  co::BinReader bin_reader(cmd_result_opaque_buf_);
  NetshellError unser_err;
  unserer.Unserialize(bin_reader, unser_err);
  //if (unser_err) {
  string new_salt;
  if (!bin_reader.ReadString(new_salt)) {
    cmdinfo_holder.SafelyChangeData([&](CmdInfo& cmd_info) {
      cmd_info.state = CmdState::kNone;
      });
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
      .WithMessageBody("new_salt read err"));
    return;
  }
  if (new_salt.length()) {
    // Set the new salt. We ARE IN AcquireCmdinfoLock/ReleaseCmdInfoLock so
    // another thread can't race
    bmdata_->StoreSalt(new_salt);
  }
  else {
    // if empty, don't change the salt
  }

  if (unser_err) {
    cmdinfo_holder.SafelyChangeData([&](CmdInfo& cmd_info) {
      cmd_info.state = CmdState::kUnserializeError;
      cmd_info.netshell_error = unser_err;
      });
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
      .WithMessageBody(string_printf("unserialize err: %s", unser_err.MakeErrorMessage().c_str())));
    return;
  }

  DCHECK(side_sections_.size() == 1 || side_sections_.size() == 2);
  string* post_section = nullptr;
  if (side_sections_.size() == 2) {
    post_section = &side_sections_[1];
  }
  if (post_section) {
    // SUCCESS, but need postprocessing
    cmdinfo_holder.SafelyChangeData([&](CmdInfo& cmd_info) {
      cmd_info.state = CmdState::kPostprocessing;
      cmd_info.bot_result = unsered_result_;
      cmd_info.post_command = *post_section;
      // All other fields are empty which shows the SUCCESS
      }); 
    // Will SetResult from HandleExecutePostCommand
  }
  else {
    // SUCCESS
    cmdinfo_holder.SafelyChangeData([&](CmdInfo& cmd_info) {
      cmd_info.state = CmdState::kDoneWithoutPostpocess;
      cmd_info.bot_result = unsered_result_;
      // All other fields are empty which shows the SUCCESS
      });
    SetResult(unsered_result_);
  }

  // <<< UNLOCK >>>

  bmdata_->ReleaseCmdLock();

  if (post_section) {
    llog() << "Executing post section\n";

    Shptr<Task> task_spawned;
    GetDispatchContext().GetCmdExecutor().ExecuteCommand(
      GetCustomApi(),
      GetDispatchContext().GetSessionApi(),
      &unsered_result_, // input_ns_result
      *post_section,
      post_result_,
      co::async::wrap_post(GetFiberStrand(),
        co::bind(&CmdexecCmdTask::HandleExecutePostCommand,
          shared_from_this(), rt)),
      task_spawned);
  }
  else {
    llog() << "No post section\n";
  }
}


void CmdexecCmdTask::HandleExecutePostCommand(RefTracker rt) {
  llog() << "HandleExecutePostCommand\n";

  auto& cmdinfo_holder = bmdata_->GetCmdInfoInterlockedHolder();
  cmdinfo_holder.SafelyChangeData([&](CmdInfo& cmd_info) {
    cmd_info.state = CmdState::kDoneWithPostprocess;
    cmd_info.post_result = post_result_;
    });
  SetResult(post_result_);
}

}}}
