#include "vlanbleed2/async/test_kit/stm/test_service_with_scripts.h"
#include "vlanbleed2/async/test_kit/stm/service_with_scripts_tester.h"
#include "vlanbleed2/async/test_kit/stm/stock.h"
//#include "vlanbleed2/async/test_kit/stm/opaque_context_stream.h"
#include "vlanbleed2/async/test_kit/stm/script_strategy_for_service.h"
#include "vlanbleed2/async/test_kit/stm/strategy_runner.h"
#include "vlanbleed2/async/test_kit/stm/script_builder.h"

#include "co/async/loop_object_set.h"
#include "co/async/tcp_service.h"
#include "co/async/thread_model.h"
#include "co/async/configs/thread_model_config_from_dict.h"

#include "co/base/csv/csv_to_tabulate.h"
#include "co/base/opaque_object.h"
#include "co/base/flags.h"
#include "co/base/textualizable.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;
using namespace co::async::configs;
using co::net::Endpoint;
using co::net::TcpEndpoint;
//using co::async::test_kit::stm; // TODO: namespaces

// must be free, participants connected/accepted on this address
static const string g_link_addr_str = "127.0.0.1:54321";


void test_co_async_services_simp(TestInfo& ti) {
  TcpService service;
  ServiceWithScriptsTester tester;
  tester.TestServiceWithScripts(
    ti.opts_dict, ConsumeAction::kDontConsume, true/*dry*/,
    g_link_addr_str, service,
    {
      {SBuilder().Write("pussy").Read(1).NoErr().S() },
      {SBuilder().Write("fish").Write("e").NoErr().S() }
    });
}

// test_co_async_services_shutdown ()
// Tests how co:: builtin services (now TcpService) behave in some operation chains
// Whether got EOF after remote shutdown, etc.
void test_co_async_services_shutdown(TestInfo& ti) {

  TcpService svc;
  TestServiceWithScriptPairs(ti.opts_dict, svc, stock::gShutdownCloseScripts);

}






