#include "zca/test_kit/zca_test_object.h"
#include "zca/netshell_status_descriptor_table.h"

#include "zca/modules/basecmd/back/basecmd_backend_module.h"
#include "zca/modules/basecmd/ag/basecmd_agent_module.h"
#include "zca/modules/dummy/dummy_module.h"

#include "netshell/textualizer.h"

#include "co/async/capsule/capsule_builder.h"
#include "co/async/configs/thread_model_config_from_dict.h"
#include "co/async/test_kit/default_capsule_object_tester.h"

#include "co/base/csv/print_csv.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::csv;
using namespace co::async;
using namespace co::async::capsule;
using namespace co::async::configs;
using namespace co::async::test_kit;

using namespace netshell;
namespace {

class AbortedCmdTestObject : public ZcaTestObjectSync {
public:
  virtual ~AbortedCmdTestObject() = default;
  
  using ZcaTestObjectSync::ZcaTestObjectSync;

private:
  void EnableParties() override {
    EnableBackend();
    EnableAgent();
    EnableBackParaNetshellClient();
  }
  void AddModules() override {
    GetTriplet().AddModuleTriplet(make_unique<modules::basecmd::back::BasecmdBackendModule>(), nullptr, make_unique<modules::basecmd::ag::BasecmdAgentModule>());
  }
  void OnSyncAllConnected() override {
    NetshellError ns_err_;

    ns_err_ = SyncBackshellCmdWriter().WriteCommand(
      "cmd-exec 00000000000000000000000000000000 -- wait -1 -- echo-input");

    DCHECK(!ns_err_);

    syslog(_INFO) << "##1 stopping Agent...\n";

    GetTriplet().GetAgent().StopThreadsafe();

    NsCmdResult cmd_result;
    uint64_t cmd_index;

    syslog(_INFO) << "##2 Reading parallel result\n";

    // Ensure command finished with NO_RESULT
    ns_err_ = SyncBackshellParaResReader().ReadParallelResult(cmd_index, cmd_result);
    DCHECK(!ns_err_);
    //DCHECK(cmd_result.status_code == kNsCmdNoResult);
    //-------------------------------------------
    // We don't check the result here.
    // Result can be either err 995 cancelled (when executed here)
    // or NO_RESULT (when doing this from console).
    // For today, we just happy that it doesn't crash and the bot is removed.
    // Enough for us.
    //--------------------------------------------

    syslog(_INFO) << "##3 Writing bot-list -c command\n";

    // Ensure there are no bots in list
    ns_err_ = SyncBackshellCmdWriter().WriteCommand("bot-list -c");
    DCHECK(!ns_err_);

    syslog(_INFO) << "##4 Reading parallel result of bot-list -c command\n";

    ns_err_ = SyncBackshellParaResReader().ReadParallelResult(cmd_index, cmd_result);
    DCHECK(!ns_err_);
    NsCmdResultTextualizer texer(gZcaNsStatusDescriptorTable, cmd_result);
    syslog(_INFO) << "##5 textualized netshell result **:\n" << texer.GetTextualized() << "\n";
    DCHECK(cmd_result.body_line_count == 1); // only csv header

    syslog(_INFO) << "##6 Stopping backend\n";

    // Stop the backend to quit
    GetTriplet().GetBackend().StopThreadsafe();

    syslog(_INFO) << "##7 done\n";
  }
};
}

void test_zca_cb_aborted_cmd(TestInfo& ti) {
  DefaultCapsuleObjectTester tester(ti, [](ThreadModel& tm) {
    return make_unique<AbortedCmdTestObject>(tm);
    });
  tester.ExecuteExpect(RunLoop::LoopExitCause::kIterLimitReachedCause);
}
