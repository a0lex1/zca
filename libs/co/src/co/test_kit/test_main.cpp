#include "co/test_kit/test_main.h"

#include "./handler_tracking.h"

#include "co/base/cmdline/keyed_cmdline.h"
#include "co/base/redirect_std_handle.h"
//#include "co/base/print_diagnostic_information.h"

#include "co/xlog/xlog.h"
#include "co/xlog/configs.h"
#include "co/base/tests.h"

//#include <crtdbg.h>

using namespace std;
using namespace co;
using namespace co::cmdline;
using namespace co::xlog;
using namespace co::xlog::configs;


// If |table_order| isn't empty, it should contain the names of all test tables.
// For samples-project, set |need_only_one| to true to force user to specify name of sample in cmdline
int test_main(int argc, char* argv[],
  const vector<pair<string, co::TestTable>>& tables,
  bool need_only_one)
{

  KeyedCmdLine<char> cl(argc, argv);

  InitHandlerTrackingIfEnabled(cl);

  try {
    LogConfigFromDict log_conf(LogConfig(), cl.GetNamedArgs(), ConsumeAction::kConsume);
    InitLogWithConfig(log_conf);

    syslog(_INFO) << "Hello from test_main. Have some work.\n";
  }
  catch (std::exception& e) {
    cout << "Exception: " << e.what() << "\n";
    //PrintBoostDiagnosticInformation(e, cout);
    return -1;
  }

  syslog(_INFO) << "Log initialized\n";

  int ret = co::RunTests(cl, tables, need_only_one);
  if (ret != 0) {
    cout << "RunTests() returned error " << ret << "\n";
  }

  // Unnecessary, but explicit uninit log
  UninitLogWithConfig();

  //_CrtDumpMemoryLeaks();

  return ret;
}


