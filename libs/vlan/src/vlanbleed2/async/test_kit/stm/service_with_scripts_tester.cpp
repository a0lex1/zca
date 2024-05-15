#include "vlanbleed2/async/test_kit/stm/service_with_scripts_tester.h"
#include "vlanbleed2/async/test_kit/stm/script_strategy_for_service.h"
#include "vlanbleed2/async/test_kit/stm/strategy_runner.h"

#include "co/async/loop_object_set.h"
#include "co/async/tcp_service.h"
#include "co/async/thread_model.h"
#include "co/async/configs/thread_model_config_from_dict.h"
#include "co/base/tests.h"
#include "co/base/csv/csv_to_tabulate.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;
using namespace co::async::configs;
using co::net::Endpoint;
using co::net::TcpEndpoint;


void ServiceWithScriptsTester::TestServiceWithScripts(
  StringMap& sm,
  ConsumeAction consume,
  bool dry_run,
  const string& link_addr_str,
  Service& service,
  const std::vector<Script>& participants
  )
{
  ThreadModelConfigFromDict tmconf(
    ThreadModelConfig(),
    sm,
    consume);

  ThreadModel tm(tmconf);
  service.SetIoContext(tm.DefIOC());

  Shptr<Strand> objset_strand = make_unique<Strand>(tm.DefIOC());
  LoopObjectSet objset(objset_strand);

  Errcode err;

  Uptr<net::AddressModel> addr_model = service.CreateAddressModel();
  Endpoint link_addr = addr_model->GetDefaultEndpoint();
  link_addr.FromString(link_addr_str/*,err*/); // throws

  syslog(_INFO) << "link_addr_ => " << link_addr.ToString() << "\n";
  syslog(_INFO) << "Forming ObjectSet with " << participants.size() << " scripts\n";

  // Form new LoopObjectSet containing strategy runners contains scripts
  bool is_server = true;
  size_t nparticip = 0;
  for (auto& particip_script : participants) {

    Uptr<ScriptStrategyForService> particip_strat = // Strategy
      make_unique<ScriptStrategyForService>(particip_script,
                                            dry_run,
                                            service,
                                            link_addr);
    // Set tag to help debugging
    if (is_server) {
      if (dry_run) {
        SET_DEBUG_TAG(particip_strat->_DbgGetTag(), "srv_%ddry", nparticip);
      }
      else {
        SET_DEBUG_TAG(particip_strat->_DbgGetTag(), "srv_%d", nparticip);
      }
    }
    else {
      if (dry_run) {
        SET_DEBUG_TAG(particip_strat->_DbgGetTag(), "cli_%ddry", nparticip);
      }
      else {
        SET_DEBUG_TAG(particip_strat->_DbgGetTag(), "cli_%d", nparticip);
      }
    }

    // In case where user didn't specify .Accept() or .Connect(), we add it
    // by default (call to ->SetRole()).
    // participants[0] initiates as server, others as clients
    auto particip_role = is_server ? ScriptStrategyForService::Role::eAccept
      : ScriptStrategyForService::Role::eConnect;
    is_server = false; // bool only first time
    particip_strat->SetRole(particip_role);

    Uptr<Stream> stm = service.CreateStreamFactory()->CreateStream();

    //Uptr<StrategyContext> opaque_context = // what is it for?
    //  make_unique<OpaqueContextStream>(
    //    std::move(stm));

    Uptr<StrategyRunner> srunner =
      make_unique<StrategyRunner>(
        std::move(particip_strat),
        //std::move(opaque_context)
        nullptr
        );

    objset.AddObject(std::move(srunner));

    nparticip += 1;
  }

  syslog(_INFO) << "Starting ObjectSet...\n";

  RefTrackerContext rtctx(CUR_LOC());
  objset.Start(RefTracker(CUR_LOC(), rtctx.GetHandle(), []() {
    syslog(_INFO) << "ObjectSet #ioended\n";
               }));

  // Run the thread model
  syslog(_INFO) << "Running thread model...\n";
  tm.Run();
  syslog(_INFO) << "Thread model Run returned\n";

  DCHECK(0 == rtctx.GetAtomicRefTrackerCount());

  syslog(_INFO) << "Displaying " << objset.GetObjectCount() << " reports\n";

  for (size_t i = 0; i < objset.GetObjectCount(); i++) {
    Shptr<StrategyRunner> strat_runner =
      static_pointer_cast<StrategyRunner>(
        objset.GetObject(i));

    // Display each script's report
    ScriptStrategy& script_strat(
      static_cast<ScriptStrategy&>(
      strat_runner->GetStrategy()));

    static const char* names[] = { "alice", "bob" };
    DCHECK(i < (sizeof(names) / sizeof(names[0])));

    DisplayScriptReport(script_strat.GetScript().GetOps(),
                        script_strat.GetOpResultVector(),
                        true,
                        names[i]);
  }

  syslog(_INFO) << "Done\n";
}

void ServiceWithScriptsTester::DisplayScriptReport(const std::vector<SOp>& ops, const std::vector<SOpRes>& op_results, bool print_header, const std::string& name)
{
  //DCHECK(ops.size() == op_results.size());
  size_t num_columns = std::max(ops.size(), op_results.size());
  std::vector<StringVector> csv;
  StringVector row;
  // Header
  if (print_header) {
    row.push_back("name");
    for (size_t i = 0; i < num_columns; i++) {
      row.push_back(string_printf("a%d", i));
    }
    csv.push_back(row);
    row.clear();
  }
  // Body - Ops
  row.push_back(name + " ops");
  for (const SOp& op : ops) {
    row.push_back(op.GetTextualized());
  }
  // extra pad
  for (size_t i = 0; i < num_columns - ops.size(); i++) {
    row.push_back("-");
  }
  csv.push_back(row);
  row.clear();
  // Body - Op Results
  row.push_back(name + " results");
  for (const SOpRes& opres : op_results) {
    row.push_back(opres.GetTextualized());
  }
  for (size_t i = 0; i < num_columns - op_results.size(); i++) {
    row.push_back("-");
  }
  csv.push_back(row);
  row.clear();

  //
  // Pretty print the CSV
  //

  std::string csv_text;
  co::csv::CsvToTabulateDump(csv, csv_text);
  syslog(_INFO) << csv_text << "\n";
}
