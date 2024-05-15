#include "cc/test_kit/server_proto_tester_runner.h"

#include "cc/cc_server.h"

#include "co/async/tcp_service.h"
#include "co/async/tcp.h"
#include "co/async/thread_model.h"
#include "co/common_config.h"
#include "co/xlog/xlog.h"

using namespace boost::posix_time;
using namespace co;
using namespace co::async;
using namespace co::net;
using namespace std;

namespace cc {
namespace test_kit {

ServerProtoTesterRunner::ServerProtoTesterRunner(ServerProtoTester& tester, Shptr<ProtoMessageFactory> protfac) :
  tester_(tester), protfac_(protfac)
{

}

void ServerProtoTesterRunner::Run()
{
  ThreadModel tm;
  TcpService svc(tm.DefIOC());

  Endpoint link_ep = TcpEndpoint("127.0.0.1:0");
  CcServer ccserv(link_ep,
                  ServerObjects(
                  make_shared<TcpStreamFactory>(tm.DefIOC()),
                  make_unique<TcpStreamAcceptor>(tm.DefIOC()),
                  tm.DefIOC()),
                  &tester_.GetEvents(),
                  common_config::kMaxChunkBodySize);

  ccserv.DisablePinging();

  // We're about to send messages from proto sockets (imitating client/server as
  // one side) so we cannot use traffic encryption. Disable it.
  ccserv.DisableTrafficEncryption();

  ccserv.PrepareToStartNofail();
  link_ep = ccserv.GetLocalAddressToConnect();
  RefTrackerContext rtctx(CUR_LOC());

  ccserv.Start(RefTracker(CUR_LOC(), rtctx.GetHandle(), []() {
    syslog(_INFO) << "cc server #ioended\n";
               }));

  tester_.SetCcServer(ccserv);

  tester_.SetService(svc);
  tester_.SetProtoMessageFactory(*protfac_.get());
  tester_.SetLinkEndpoint(link_ep);

  tester_.Initiate([&]() {
    syslog(_INFO) << "ServerProtoTester #ioended, stopping CcServer\n";
    ccserv.StopThreadsafe();
                   });

  tm.Run();

  // we must have been stopped gracefully, without ioc.stop()
  DCHECK(0 == rtctx.GetAtomicRefTrackerCount());

  // See Cleanup() comments.
  tester_.Cleanup();
}

}}
