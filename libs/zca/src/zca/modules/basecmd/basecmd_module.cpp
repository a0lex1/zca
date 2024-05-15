#include "zca/modules/basecmd/basecmd_module.h"
#include "zca/modules/basecmd/wait_cmd_task.h"

using namespace netshell;

namespace modules {
namespace basecmd {

BasecmdModule::BasecmdModule() {
  disp_table_.AddCommand("echo-args", co::bind(&BasecmdModule::DispatchEchoargsCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("echo-input", co::bind(&BasecmdModule::DispatchEchoinputCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("wait", co::bind(&BasecmdModule::DispatchWaitCmd, this, _1, _2, _3, _4));
}

void BasecmdModule::DispatchEchoargsCmd(DispatchContext& disp_context,
                                      DispatchCmdData& disp_data,
                                      Shptr<Strand> strand,
                                      Shptr<DispatchCmdTask>& new_task) {
  DCHECK(new_task == nullptr);

  auto& cmd = disp_data.GetNsCommand();
  StringVector args;
  co::cmdline::ParseCmdlineToArgv(cmd.c_str(), args);

  // Remove first part (program name). The rest are args. Echo them away from here.
  args.erase(args.begin());

  disp_data.GetNsResult() = NsCmdResult(kNsCmdExecuted, 0, NsResultType::kText).WithTextBody(args);

  // |new_task| hasn't been set means the task is immediately completed.
}

void BasecmdModule::DispatchEchoinputCmd(DispatchContext& disp_context,
                                       DispatchCmdData& disp_data,
                                       Shptr<Strand> strand,
                                       Shptr<DispatchCmdTask>& new_task) {
  DCHECK(new_task == nullptr);

  auto& cmd = disp_data.GetNsCommand();
  StringVector args;
  co::cmdline::ParseCmdlineToArgv(cmd.c_str(), args);

  if (args.size() == 1) {
    // OK, no args
    if (disp_data.GetInputNsResult() != nullptr) {
      // Echo input result as command result
      disp_data.GetNsResult() = *disp_data.GetInputNsResult();
    }
    else {
      disp_data.GetNsResult() = NsCmdResult(kNsCmdExecuted, 0, NsResultType::kMessage)
        .WithMessageBody("This command should be executed with input result");
    }
  }
  else {
    disp_data.GetNsResult() = NsCmdResult(kNsCmdExecuted, 0, NsResultType::kMessage)
      .WithMessageBody("No args allowed, only input");
  }
}

void BasecmdModule::DispatchWaitCmd(DispatchContext& disp_context,
                                  DispatchCmdData& disp_data,
                                  Shptr<Strand> strand,
                                  Shptr<DispatchCmdTask>& new_task) {
  DCHECK(new_task == nullptr);
  // Schedule new task
  new_task = make_shared<WaitCmdTask>(
    make_shared<Strand>(disp_context.GetSessionApi().GetSessionIoContext()));
}

}}

  