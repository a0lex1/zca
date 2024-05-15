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

  class ParallelCmdTestObject : public ZcaTestObjectSync {
  public:
    virtual ~ParallelCmdTestObject() = default;

    //using ZcaTestObjectSync::ZcaTestObjectSync;
    ParallelCmdTestObject(ThreadModel& tm, size_t total_loop,
      const StringVector& cmds_to_send,
      vector<NsCmdResult>& cmd_results,
      bool use_backend_instead_of_frontend, bool disable_sig_check=false)
      :
      ZcaTestObjectSync(tm),
      total_loop_(total_loop),
      cmds_to_send_(cmds_to_send), cmd_results_(cmd_results),
      use_backend_instead_of_frontend_(use_backend_instead_of_frontend),
      disable_sig_check_(disable_sig_check)
    {

    }

    // public:
    vector<NsCmdResult>& cmd_results_;
    size_t cmd_counter_;

  private:
    size_t total_loop_;
    bool use_backend_instead_of_frontend_;
    bool disable_sig_check_;

    StringVector cmds_to_send_;

  private:
    void EnableParties() override {
      EnableBackend();
      EnableAgent();
      EnableFrontend();
      if (use_backend_instead_of_frontend_) {
        EnableBackParaNetshellClient(cmds_to_send_.size());
      }
      else {
        EnableFrontNetshellClient(cmds_to_send_.size());
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

    NetshellError WriteCommand(size_t nsession, const string& command) { 
      if (use_backend_instead_of_frontend_) {
        return SyncBackshellCmdWriter(nsession).WriteCommand(command);
      }
      else {
        return SyncFrontshellCmdWriter(nsession).WriteCommand(command);
      }
    }

    NetshellError ReadResult(size_t nsession, NsCmdResult& cmd_result) {
      if (use_backend_instead_of_frontend_) {
        return SyncBackshellParaResReader(nsession).ReadParallelResult(cmd_counter_, cmd_result);
      }
      else {
        return SyncFrontshellResReader(nsession).ReadResult(cmd_result);
      }
    }

    void OnSyncAllConnected() override {
      uint64_t cmd_index;
      NetshellError ns_err;

      for (size_t nloop=0; nloop<total_loop_; nloop++) {

        cmd_results_.clear();

        for (size_t nsession=0; nsession<cmds_to_send_.size(); nsession++) {
          ns_err = WriteCommand(nsession, cmds_to_send_[nsession]);
          if (ns_err) {
            break;
          }
        }
        for (size_t nsession=0; nsession<cmds_to_send_.size(); nsession++) {
          NsCmdResult ns_result;
          ns_err = ReadResult(nsession, ns_result);
          if (ns_err) {
            break;
          }
          cmd_results_.emplace_back(ns_result);
        }

        DCHECK(cmds_to_send_.size() == cmd_results_.size());
      }

      GetTriplet().GetBackend().StopThreadsafe();
      GetTriplet().GetFrontend().StopThreadsafe();
    }
  };
}

// Flood with two parallel commands: cmd-exec and bot-list
// This test is to detect cmdinfo_property_group RaceCond(s)
void test_zca_cbf_cmd_floodbotlist(TestInfo& ti) {
  NsCmdResultVector cmd_results;
  StringVector cmds = {
    "bot-list -c",
    "cmd-exec 00000000000000000000000000000000 echo-args sex drugs complications | echo-input"
  };
  DefaultCapsuleObjectTester tester(ti, [&](ThreadModel& tm) {
    return make_unique<ParallelCmdTestObject>(tm,
    55,
    cmds,
    cmd_results,
    kUseFrontend);
    }
    );//-1, 9);
  tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);

  DCHECK(cmd_results.size() == 2);
  
  DCHECK(cmd_results[1].result_type == NsResultType::kText);
  DCHECK(cmd_results[1].body_line_count == 3);
  DCHECK(cmd_results[1].text_lines[0] == "sex");
  DCHECK(cmd_results[1].text_lines[1] == "drugs");
  DCHECK(cmd_results[1].text_lines[2] == "complications");
  // OK, Don't check cmd_results[0], because it may contain EMPTY NsCmdResult
  // in case bot-list command executed BEFORE cmd-exec.
  //size_t p;
  //p = FindStringIndex(cmd_results[0].csv_rows[0], "cmdstate");
  //DCHECK(cmd_results[0].csv_rows[1][p] == "DONE_NO_POST");
}
