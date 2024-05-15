#include "zca/test_kit/zca_test_object.h"

#include "zca/modules/basecmd/ag/basecmd_agent_module.h"
#include "zca/modules/basecmd/back/basecmd_backend_module.h"
#include "zca/modules/dummy/dummy_module.h"

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

  class BotlistTestObject : public ZcaTestObjectSync {
  public:
    virtual ~BotlistTestObject() = default;

    //using ZcaTestObjectSync::ZcaTestObjectSync;
    BotlistTestObject(ThreadModel& tm, const string& cmd_to_send, NsCmdResult& cmd_result,
      bool add_dummy_module=false)
      :
      ZcaTestObjectSync(tm),
      cmd_to_send_(cmd_to_send), cmd_result_(cmd_result), add_dummy_module_(add_dummy_module)
    {

    }

    NsCmdResult& cmd_result_;

  private:
    string cmd_to_send_;
    bool add_dummy_module_;

  private:
    void EnableParties() override {
      EnableBackend();
      EnableAgent();
      EnableBackParaNetshellClient();
    }
    void AddModules() override {
      GetTriplet().AddModuleTriplet(make_unique<modules::basecmd::back::BasecmdBackendModule>(), nullptr, make_unique<modules::basecmd::ag::BasecmdAgentModule>());
      if (add_dummy_module_) {
        GetTriplet().AddModuleTriplet(make_unique<modules::dummy::DummyBackendModule>(), nullptr, make_unique<modules::dummy::DummyAgentModule>());
      }
    }
    void OnSyncAllConnected() override {
      uint64_t cmd_index;
      NetshellError ns_err;

      ns_err = SyncBackshellCmdWriter().WriteCommand(cmd_to_send_);
      DCHECK(!ns_err);

      ns_err = SyncBackshellParaResReader().ReadParallelResult(cmd_index, cmd_result_);
      DCHECK(!ns_err);

      DCHECK(cmd_index == 0);

      GetTriplet().GetBackend().StopThreadsafe();
    }
  };
}

// Test

void test_zca_basecmd_cb_botlist(TestInfo& ti) {
  NsCmdResult cmd_result;
  DefaultCapsuleObjectTester tester(ti, [&](ThreadModel& tm) {
    return make_unique<BotlistTestObject>(tm, "bot-list -c", cmd_result);
    });
  tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);

  DCHECK(cmd_result.result_type == NsResultType::kCsv);
  DCHECK(cmd_result.body_line_count == 2); // csv header line + bot line
  DCHECK(cmd_result.csv_rows[0][0] == "bid");
}

// Test that DummyModule adds columns to bot-list

void test_zca_basecmd_cb_botlist_dummyfields(TestInfo& ti) {
  NsCmdResult cmd_result;
  DefaultCapsuleObjectTester tester(ti, [&](ThreadModel& tm) {
    return make_unique<BotlistTestObject>(tm, "bot-list -c", cmd_result, true);
    });
  tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);

  DCHECK(cmd_result.result_type == NsResultType::kCsv);
  DCHECK(cmd_result.body_line_count == 2); // csv header line + bot line
  size_t last_idx = cmd_result.csv_rows[0].size() - 1;
  DCHECK(cmd_result.csv_rows[0][last_idx - 1] == "pen_len");
  DCHECK(cmd_result.csv_rows[0][last_idx] == "med_dose");
}

// Test Include
void test_zca_basecmd_cb_field_include(TestInfo& ti) {
  NsCmdResult cmd_result;
  DefaultCapsuleObjectTester tester(ti, [&](ThreadModel& tm) {
    return make_unique<BotlistTestObject>(tm, "bot-list -c -a postres -a neterr", cmd_result);
    });
  tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);

  DCHECK(cmd_result.result_type == NsResultType::kCsv);
  DCHECK(cmd_result.body_line_count == 2); // csv header line + bot line
  DCHECK(cmd_result.csv_rows.size() == 2);
  DCHECK(cmd_result.csv_rows[0][0] == "postres");
  DCHECK(cmd_result.csv_rows[0][1] == "neterr");
}

// Test Exclude
void test_zca_basecmd_cb_field_exclude(TestInfo& ti) {
  NsCmdResult cmd_result;
  DefaultCapsuleObjectTester tester(ti, [&](ThreadModel& tm) {
    return make_unique<BotlistTestObject>(tm, "bot-list -c -b postres -b neterr", cmd_result);
    });
  tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);

  DCHECK(cmd_result.result_type == NsResultType::kCsv);
  DCHECK(cmd_result.body_line_count == 2); // csv header line + bot line
  // We should get everything *ELSE* than postres,stcode
  for (size_t i = 0; i < cmd_result.csv_rows[0].size(); i++) {
    if (string(cmd_result.csv_rows[0][i]) == "postres" ||
        string(cmd_result.csv_rows[0][i]) == "neterr")
      {
        NOTREACHED();
      }
  }
}

