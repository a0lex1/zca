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
    CmdTestObject(ThreadModel& tm, const StringVector& cmds_to_send,
      vector<NsCmdResult>& cmd_results,
      bool use_backend_instead_of_frontend, bool disable_sig_check=false)
      :
      ZcaTestObjectSync(tm),
      cmds_to_send_(cmds_to_send), cmd_results_(cmd_results),
      use_backend_instead_of_frontend_(use_backend_instead_of_frontend),
      disable_sig_check_(disable_sig_check)
    {

    }

    // public:
    vector<NsCmdResult>& cmd_results_;
    size_t cmd_counter_;

  private:
    bool use_backend_instead_of_frontend_;
    bool disable_sig_check_;

    StringVector cmds_to_send_;

  private:
    NetshellError WriteCommand(const string& command) { 
      if (use_backend_instead_of_frontend_) {
        return SyncBackshellCmdWriter().WriteCommand(command);
      }
      else {
        return SyncFrontshellCmdWriter().WriteCommand(command);
      }
    }
    NetshellError ReadResult(NsCmdResult& cmd_result) {
      if (use_backend_instead_of_frontend_) {
        return SyncBackshellParaResReader().ReadParallelResult(cmd_counter_, cmd_result);
      }
      else {
        return SyncFrontshellResReader().ReadResult(cmd_result);
      }
    }

    void EnableParties() override {
      EnableBackend();
      EnableAgent();
      EnableFrontend();
      if (use_backend_instead_of_frontend_) {
        EnableBackParaNetshellClient();
      }
      else {
        EnableFrontNetshellClient();
      }
    }
    void AddModules() override {
      GetTriplet().AddModuleTriplet(
        make_unique<modules::basecmd::back::BasecmdBackendModule>(),
        make_unique<modules::basecmd::front::BasecmdFrontendModule>(),
        make_unique<modules::basecmd::ag::BasecmdAgentModule>());
    }
    void SetOptions() override {
      if (disable_sig_check_) {
        GetTriplet().GetAgentConfig().disable_cmd_sig_check = true;
      }
    }
    void OnSyncAllConnected() override {
      uint64_t cmd_index;
      NetshellError ns_err;

      for (const auto& cmd_to_send : cmds_to_send_) {
        ns_err = WriteCommand(cmd_to_send);
        if (ns_err) {
          break;
        }

        NsCmdResult ns_result;
        ns_err = ReadResult(ns_result);
        if (ns_err) {
          break;
        }
        cmd_results_.push_back(ns_result);
      }
      DCHECK(cmds_to_send_.size() == cmd_results_.size());

      GetTriplet().GetBackend().StopThreadsafe();
      GetTriplet().GetFrontend().StopThreadsafe();
    }
  };
}

// -----------------------------------------------------------------------------------------

// First, ensure that we *CANNOT* execute unsigned cmds without kDisableSigCheck

void test_zca_cb_cmd_needsig(TestInfo& ti) {
  NsCmdResultVector cmd_results;
  StringVector cmds = {
    "cmd-exec 00000000000000000000000000000000 echo-args sex drugs complications",
  };
  DefaultCapsuleObjectTester tester(ti, [&](ThreadModel& tm) {
    return make_unique<CmdTestObject>(tm,
    cmds,
    cmd_results,
    kUseBackendInsteadOfFrontend);
  // Don't provide kDisableSigCheck here
    });
  tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);

  // Should not be executed
  DCHECK(cmd_results.size() == 1);
  DCHECK(cmd_results[0].result_type == NsResultType::kMessage);
  DCHECK(cmd_results[0].message == "Need ^^");
}


// Try with wrong sig

void test_zca_cb_cmd_badsig(TestInfo& ti) {
  vector<NsCmdResult> cmd_results;
  StringVector cmds = {
    "cmd-exec -x -y 00000000000000000000000000000000 echo-args sex drugs complications ^^ BADSIGN"
  };
  DefaultCapsuleObjectTester tester(ti, [&](ThreadModel& tm) {
    return make_unique<CmdTestObject>(tm,
    cmds,
    cmd_results,
    kUseBackendInsteadOfFrontend);
    });
  tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);
  DCHECK(cmd_results.size() == 1);
  DCHECK(cmd_results[0].result_type == NsResultType::kMessage);
  DCHECK(cmd_results[0].message == "Bad ^^");
}

// Use backend with *NO-SIG-CHECK* from bot side, cmd execution should succeed
void test_zca_cb_cmd(TestInfo& ti) {
  vector<NsCmdResult> cmd_results;
  StringVector cmds = {
    "cmd-exec -x -y 00000000000000000000000000000000 echo-args sex drugs complications"
  };
  DefaultCapsuleObjectTester tester(ti, [&](ThreadModel& tm) {
    return make_unique<CmdTestObject>(tm,
      cmds,
      cmd_results,
      kUseBackendInsteadOfFrontend,
      kDisableSigCheck);
    });
  tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);
  DCHECK(cmd_results.size() == 1);
  DCHECK(cmd_results[0].result_type == NsResultType::kText);
  DCHECK(cmd_results[0].body_line_count == 3);
  DCHECK(cmd_results[0].text_lines[0] == "sex");
  DCHECK(cmd_results[0].text_lines[1] == "drugs");
  DCHECK(cmd_results[0].text_lines[2] == "complications");
}

// *NO-SIG-CHECK*, with *POSTPROCESSING*
void test_zca_cb_cmd_pprocess(TestInfo& ti) {
  NsCmdResultVector cmd_results;
  StringVector cmds = {
    "cmd-exec -x 00000000000000000000000000000000 echo-args sex '|' drugs complications | echo-input",
    "bot-list -c"
  };
  DefaultCapsuleObjectTester tester(ti, [&](ThreadModel& tm) {
    return make_unique<CmdTestObject>(tm,
      cmds,
      cmd_results,
      kUseBackendInsteadOfFrontend,
      kDisableSigCheck);
    });
  tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);

  DCHECK(cmd_results.size() == 2);
  DCHECK(cmd_results[0].result_type == NsResultType::kText);
  DCHECK(cmd_results[0].body_line_count == 4);
  DCHECK(cmd_results[0].text_lines[0] == "sex");
  DCHECK(cmd_results[0].text_lines[1] == "|");
  DCHECK(cmd_results[0].text_lines[2] == "drugs");
  DCHECK(cmd_results[0].text_lines[3] == "complications");
  DCHECK(cmd_results[1].result_type == NsResultType::kCsv);
  DCHECK(cmd_results[1].body_line_count == 2);
  size_t p;
  p = FindStringIndex(cmd_results[1].csv_rows[0], "cmdstate");
  DCHECK(cmd_results[1].csv_rows[1][p] == "DONE_WITH_POST");
  p = FindStringIndex(cmd_results[1].csv_rows[0], "postcmd");
  DCHECK(cmd_results[1].csv_rows[1][p] == "echo-input");
}


// Just use frontend
void test_zca_cbf_cmd(TestInfo& ti) {
  NsCmdResultVector cmd_results;
  StringVector cmds = {
    "bot-list -c",
    "cmd-exec 00000000000000000000000000000000 echo-args sex drugs complications"
  };
  DefaultCapsuleObjectTester tester(ti, [&](ThreadModel& tm) {
    return make_unique<CmdTestObject>(tm,
      cmds,
      cmd_results,
      kUseFrontend);
    });
  tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);

  DCHECK(cmd_results.size() == 2);
  DCHECK(cmd_results[1].result_type == NsResultType::kText);
  DCHECK(cmd_results[1].body_line_count == 3);
  DCHECK(cmd_results[1].text_lines[0] == "sex");
  DCHECK(cmd_results[1].text_lines[1] == "drugs");
  DCHECK(cmd_results[1].text_lines[2] == "complications");
}

