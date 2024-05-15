#include "zca/modules/basecmd/front/cmdexec_subtask.h"

#include "co/async/wrap_post.h"

using namespace netshell;

namespace modules {
namespace basecmd {
namespace front {

CmdexecSubtask::CmdexecSubtask(
  Shptr<Strand> strand,
  core::front::AdminSessionCustomApi& custom_api,
  engine::EngineSessionApi& sess_api,
  engine::CmdExecutor& cmd_executor,
  const std::string& back_cmdline,
  const std::string& fpost_cmdline)
  :
  TaskImpl(strand),
  custom_api_(custom_api),
  sess_api_(sess_api),
  cmd_executor_(cmd_executor),
  back_cmdline_(back_cmdline),
  fpost_cmdline_(fpost_cmdline)
{

}

void CmdexecSubtask::BeginIo(co::RefTracker rt) {
  rt.SetReferencedObject(shared_from_this());

  auto& para_executor(custom_api_.GetBackshellParaExecutor());

  custom_api_.GetBackshellParaExecutor().ExecuteCommand(back_cmdline_, cmd_result_,
    wrap_post(GetFiberStrand(), co::bind(&CmdexecSubtask::HandleExecute,
      shared_from_this(), _1, rt)));

}

void CmdexecSubtask::HandleExecute(netshell::NetshellError ns_err, RefTracker rt) {
  if (fpost_cmdline_.length()) {
    Shptr<Task> task_spawned;
    cmd_executor_.ExecuteCommand(
      &custom_api_,
      sess_api_,
      &cmd_result_, // input result
      fpost_cmdline_,
      fpost_result_,
      wrap_post(GetFiberStrand(), co::bind(&CmdexecSubtask::HandleFPostExecute,
        shared_from_this())),
      task_spawned);
  }
  else {
    // Completed
  }
}

void CmdexecSubtask::HandleFPostExecute() {
  //TODO: remember fpost result
}

const NsCmdResult& CmdexecSubtask::GetFinalResult() const {
  if (!fpost_cmdline_.length()) {
    return cmd_result_;
  }
  else {
    return fpost_result_;
  }
}

void CmdexecSubtask::StopUnsafeExtra() {

}

}}}
