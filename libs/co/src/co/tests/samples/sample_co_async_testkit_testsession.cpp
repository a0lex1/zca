#include "co/xlog/xlog.h"
#include "co/async/client.h"
#include "co/async/test_kit.h"
#include "co/async/tcp.h"
#include "co/base/tests.h"
#include "co/net/asio_endpoint_from_string.h"
#include "co/base/dict.h"
#include <iostream>

using namespace co;
using namespace co::async;
using namespace co::async::test_kit;
using namespace std;
using namespace boost::asio;
using co::net::Endpoint;

namespace co {
namespace async {
namespace test_kit {

struct AsyncTestConfig {
  Endpoint addr;
};

}}}

/*
  bool read_not_write;
  size_t portion_size;
  size_t portion_count;
  boost::posix_time::time_duration portion_delay;
  char buffer_fill_char;
  */

void sample_co_async_testkit_testsession(TestInfo& info) {

}

/*
* 
* // DO I REALLY NEED THIS????????????
* 
void sample_co_async_testkit_testsession(co::TestInfo& info) {

  TcpEndpoint addr;
  TestSessionParams params(TestSessionParams::Write());
  size_t delay_msec;
  params.buffer_fill_char = 'a';
  OverrideFromDict<string, string, TcpEndpoint>(info.opts_dict, "addr", addr);
  OverrideFromDict<string, string, size_t>(info.opts_dict, "portion-size", params.portion_size, false, true);
  OverrideFromDict<string, string, size_t>(info.opts_dict, "portion-count", params.portion_count, false, true);
  OverrideFromDict<string, string, size_t>(info.opts_dict, "portion-delay-msec", delay_msec, false, true);
  params.portion_delay = boost::posix_time::milliseconds(delay_msec);

  SysLog(_INFO)
    << "addr to connect&write: " << addr.GetAddr().address().to_string() << ":" << addr.GetAddr().port() << "\n"
    << "TestSessionParams:\n"
    << "\tportion_size: " << params.portion_size << "\n"
    << "\tportion_count: " << params.portion_count << "\n"
    << "\tportion_delay (msec): " << params.portion_delay.total_milliseconds() << "\n\n";

  io_context ioc;

  Client client(
    addr,
    make_shared<TestClientSession>(params),
    make_unique<TcpStream>(ioc),
    make_shared<TcpStreamConnector>()
  );

  client.Start(RefTracker([&](Errcode err) {
    if (client.GetConnectError()) {
      SysLog(_ERR) << "Can't connect, error " << client.GetConnectError() << "\n";
    }
    else {
      SysLog(_INFO) << "Client stopped, err " << err << "\n";
    }
  }));

  ioc.run();

  SysLog(_INFO) << "ioc.run() returned\n";
}


*/








