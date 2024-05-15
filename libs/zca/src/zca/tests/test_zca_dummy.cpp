#include "zca/netshell_status_descriptor_table.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

#include "zca/test_kit/zca_test_object.h"

#include "zca/modules/basecmd/back/basecmd_backend_module.h"
#include "zca/modules/basecmd/ag/basecmd_agent_module.h"

#include "co/async/test_kit/default_capsule_object_tester.h"

#include "co/base/csv/print_csv.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;
using namespace co::async::test_kit;

using namespace netshell;
namespace {

class EchoargsTestObject : public ZcaTestObjectSync {
public:
  virtual ~EchoargsTestObject() = default;
  
  using ZcaTestObjectSync::ZcaTestObjectSync;

private:
  void EnableParties() override {
    EnableBackend();
    EnableAgent();
    EnableBackParaNetshellClient();
  }
  void SetOptions() override { // Leave it here to bind
    GetTriplet().GetAgentConfig().bot_id = cc::BotId::FromUint(0x1338);
    // If needed to debug hanging test with nc:
    //GetTriplet().SetBackendPort(10000);
    //GetTriplet().SetBackendCcPort(20000);
  }
  void AddModules() override {
    GetTriplet().AddModuleTriplet(
      make_unique<modules::basecmd::back::BasecmdBackendModule>(),
      nullptr,
      make_unique<modules::basecmd::ag::BasecmdAgentModule>());
  }
  void OnSyncAllConnected() override {
    NetshellError ns_err;
    NsCmdResult cmd_result;
    uint64_t cmd_index;

    //SyncAuxTimer().WaitFor(boost::posix_time::milliseconds(5));
    
    ns_err = SyncBackshellCmdWriter().WriteCommand("echo-args 1 2 3 AAA BBB CCC");
    DCHECK(!ns_err);
    ns_err = SyncBackshellParaResReader().ReadParallelResult(cmd_index, cmd_result);
    DCHECK(!ns_err);
    DCHECK(cmd_index == 0);

    DCHECK(cmd_result.status_code == kNsCmdExecuted);
    DCHECK(cmd_result.ret_code == 0);
    DCHECK(cmd_result.result_type == NsResultType::kText);
    DCHECK(cmd_result.text_lines == StringVector({ "1", "2", "3", "AAA", "BBB", "CCC" }));

    GetTriplet().GetBackend().StopThreadsafe();
  }
};
}

void test_zca_echoargs(TestInfo& ti) {
  DefaultCapsuleObjectTester tester(ti, [](ThreadModel& tm) {
    return make_unique<EchoargsTestObject>(tm);
                               });
  tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);
}




