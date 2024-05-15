#include "zca/modules/basecmd/front/kill_cmd_task.h"

#include "zca/core/front/admin_session_custom_api.h"
#include "zca/core/res_with_msg_body.h"

using namespace std;
using namespace co;
using namespace co::async;
using namespace netshell;

namespace modules {
namespace basecmd {
namespace front {

KillCmdTask::KillCmdTask(Shptr<Strand> strand)
  : DispatchCmdTask(strand)
{
}

void KillCmdTask::BeginIo(RefTracker rt) {
  rt.SetReferencedObject(shared_from_this());

  auto& para_executor(GetCustomApiAs<core::front::AdminSessionCustomApi>()->GetBackshellParaExecutor());

  para_executor.ExecuteCommand(GetNsCommand(), back_result_,
    wrap_post(GetFiberStrand(), co::bind(&KillCmdTask::HandleBackExecute,
      shared_from_this(), _1, rt)));
}

void KillCmdTask::HandleBackExecute(NetshellError ns_err, RefTracker rt) {
  if (ns_err) {
    SetResult(core::ResWithMsgBody(
      -1, string_printf("backend returned unexpected res type: %d", back_result_.result_type)));
    return;
  }
  SetResult(back_result_);
}

void KillCmdTask::StopUnsafeExtra() {
}

}}}
