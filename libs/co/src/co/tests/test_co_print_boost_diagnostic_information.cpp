#include "co/base/tests.h"
#include "co/base/print_diagnostic_information.h"
#include "co/xlog/configs.h"

using namespace std;
using namespace co;
using namespace co::xlog::configs;

void test_co_print_boost_diagnostic_information(TestInfo&) {

  try {
    StringMap sss{ {"log-sevs", "sss"} };
    LogConfigFromDict lo(LogConfig(), sss, ConsumeAction::kConsume);
  }
  catch (ConfigException& e) {
    PrintBoostDiagnosticInformation(e, "sss_function", cout);
    syslog(_INFO) << "just did it just to print to console\n";
  }
  return;
}


