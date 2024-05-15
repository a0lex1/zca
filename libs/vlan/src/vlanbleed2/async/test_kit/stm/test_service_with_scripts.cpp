#include "vlanbleed2/async/test_kit/stm/test_service_with_scripts.h"
#include "vlanbleed2/async/test_kit/stm/service_with_scripts_tester.h"

#include "co/async/tcp_service.h"
#include "co/async/tcp.h"

#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;

// must be free, participants connected/accepted on this address
static const string g_link_addr_str = "127.0.0.1:54321";


void TestServiceWithScriptPairs(StringMap& opts_dict,
                                Service& svc,
                                const vector<pair<Script, Script>>& pairs)
{
  syslog(_INFO) << "1. DRY RUN.\n";

  size_t i = 0;
  for (const auto& script : pairs) {
    syslog(_INFO) << "Testing script pairs[" << i << "] in[!] DRY RUN mode[!]\n";

    TcpService service;
    ServiceWithScriptsTester tester;
    tester.TestServiceWithScripts(
      opts_dict, ConsumeAction::kDontConsume, true/*dry*/,
      g_link_addr_str, service,
      { script.first, script.second });

    syslog(_INFO) << "\n";

    i += 1;
  }
  syslog(_INFO) << "\n\n\n=============================\n"
    << "2. REAL MODE, NO DRY RUN.\n===========================\n\n\n\n\n";

  i = 0;
  for (const auto& script : pairs) {
    syslog(_INFO) << "Testing script pairs[" << i << "] in REAL MODE\n";

    TcpService service;
    ServiceWithScriptsTester tester;

    tester.TestServiceWithScripts(
      opts_dict, ConsumeAction::kDontConsume, false/*dry*/,
      g_link_addr_str, service,
      { script.first, script.second });

    syslog(_INFO) << "\n";

    i += 1;
  }
  syslog(_INFO) << "2 - DONE REAL MODE.\n\n\n\n";
}
