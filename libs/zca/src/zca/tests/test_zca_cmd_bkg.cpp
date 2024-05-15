#include "zca/test_kit/zca_test_object.h"

#include "zca/modules/basecmd/ag/basecmd_agent_module.h"
#include "zca/modules/basecmd/back/basecmd_backend_module.h"
#include "zca/modules/basecmd/front/basecmd_frontend_module.h"

#include "zca/modules/dummy/dummy_module.h"

#include "co/async/capsule/capsule_builder.h"
#include "co/async/configs/thread_model_config_from_dict.h"
#include "co/async/test_kit/default_capsule_object_tester.h"

#include "co/base/find_string_index.h"
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
  static constexpr bool kUseBackendInsteadOfFrontend = true;
  static constexpr bool kUseFrontend = false;
  static constexpr bool kDisableSigCheck = true;

  class CmdTestObject : public ZcaTestObjectSync {
  public:
    virtual ~CmdTestObject() = default;

    //using ZcaTestObjectSync::ZcaTestObjectSync;
    CmdTestObject(ThreadModel& tm, const string& cmd,
      NsCmdResult& cmd_result)
      :
      ZcaTestObjectSync(tm),
      cmd_(cmd), cmd_result_(cmd_result)
    {

    }

    // public:
    NsCmdResult& cmd_result_;
    size_t cmd_counter_;

  private:
    string cmd_;

  private:
    void EnableParties() override {
      EnableBackend();
      EnableAgent();
      EnableFrontend();
      EnableFrontNetshellClient();
    }
    void AddModules() override {
      GetTriplet().AddModuleTriplet(
        make_unique<modules::basecmd::back::BasecmdBackendModule>(),
        make_unique<modules::basecmd::front::BasecmdFrontendModule>(),
        make_unique<modules::basecmd::ag::BasecmdAgentModule>());
    }
    void OnSyncAllConnected() override {
      uint64_t cmd_index;
      NetshellError ns_err;

      ns_err = SyncFrontshellCmdWriter().WriteCommand(cmd_);
      DCHECK(!ns_err);

      ns_err = SyncFrontshellResReader().ReadResult(cmd_result_);
      DCHECK(!ns_err);

      GetTriplet().GetBackend().StopThreadsafe();
      GetTriplet().GetFrontend().StopThreadsafe();
    }
  };
}

// -----------------------------------------------------------------------------------------

void test_zca_cmd_bkg(TestInfo& ti) {
  NsCmdResult cmd_result;
  string cmd = "cmd-exec -n 00000000000000000000000000000000 wait -1";
  DefaultCapsuleObjectTester tester(ti, [&](ThreadModel& tm) {
    return make_unique<CmdTestObject>(tm, cmd, cmd_result);
    });
  tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);

  // ...
}

