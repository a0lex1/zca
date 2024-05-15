#include "zca/modules/basecmd/back/basecmd_backend_module.h"
#include "zca/modules/basecmd/ag/basecmd_agent_module.h"
#include "zca/modules/dummy/dummy_module.h"

#include "zca/test_kit/cmd/cmdexec_test_object.h"

#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;
using namespace boost::posix_time;
using namespace netshell;

CmdexecTestObject::CmdexecTestObject(ThreadModel& tm, const CmdexecTestObjectParams& pams,
                                     bool dry_run)
  : ZcaTestObjectSync(tm), pams_(pams), dry_(dry_run)
{

}

void CmdexecTestObject::EnableParties()
{
  EnableBackend();
  EnableAgent();
  EnableBackParaNetshellClient();
}

void CmdexecTestObject::SetOptions()
{
  // Comment out. Don't let them use random ports. The new code will use
  // test_kit:: test tcp endpoint's port.
  //GetTriplet().SetBackendPort();
  //GetTriplet().SetBackendCcPort();
  pams_.bot_id = cc::BotId::FromUint(0);
  syslog(_INFO) << "generated bot_id_ " << pams_.bot_id.ToStringRepr() << "\n";
}

void CmdexecTestObject::AddModules()
{
  GetTriplet().AddModuleTriplet(make_unique<modules::basecmd::back::BasecmdBackendModule>(),
                                nullptr,
                                make_unique<modules::basecmd::ag::BasecmdAgentModule>());
}

void CmdexecTestObject::OnSyncAllConnected() {
  NetshellError ns_err;
  NsCmdResult cmd_result;
  uint64_t cmd_index;
  string s_bot_id(pams_.bot_id.ToStringRepr());

  syslog(_INFO) << "EXECUTING COMMAND\n";

  ns_err = SyncBackshellCmdWriter().WriteCommand(pams_.cmd);
  DCHECK(!ns_err);
  ns_err = SyncBackshellParaResReader().ReadParallelResult(cmd_index, cmd_result);
  DCHECK(!ns_err);
  DCHECK(cmd_index == 0);
  NsCmdResultTextualizer texer(gZcaNsStatusDescriptorTable, cmd_result);
  syslog(_INFO) << "CMD RESULT =>>\n"
                << texer.GetTextualized(); //<< "\n";

  if (!dry_) {
    syslog(_INFO) << "COMPARING RESULTS ...\n";
    // Compare results
    DCHECK(pams_.resultchk_fn != nullptr);

    pams_.resultchk_fn(cmd_result);

    DCHECK(pams_.ccerrstackchk_fn != nullptr);

    pams_.ccerrstackchk_fn(GetTriplet().GetAgent().GetCcClientLastErrorStack());
  }
  else {
    syslog(_INFO) << "DRY RUN - NO COMPARING RESULTS\n";
  }

  // If needed, delay before stopping backend
  if (pams_.delay_before_stop != time_duration()) {
    syslog(_INFO) << "final_sleep_ not 0, sleeping " << pams_.delay_before_stop << " ... ... ...\n";
    DCHECK_NE(SyncAuxTimer().WaitFor(pams_.delay_before_stop));
    syslog(_INFO) << "FINAL TIMER WAIT DONE\n";
  }
  syslog(_INFO) << "STOPPING BACKEND\n";
  GetTriplet().GetBackend().StopThreadsafe();
  //AbortWaitingHandshake();
  
  syslog(_INFO) << "backend StopThreadsafe() button pressed, returning from [CORO] OnBotHandshakedNetshellReady().\n";
}






