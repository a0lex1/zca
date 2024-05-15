#include "cc/test_kit/client_proto_tester_runner.h"

#include "cc/cc_client.h"

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

ClientProtoTesterRunner::ClientProtoTesterRunner(ClientProtoTester& tester, 
  Shptr<ProtoMessageFactory> protfac)
  :
  tester_(tester), protfac_(protfac),
  svc_(tm_.DefIOC()),
  rtctx_(CUR_LOC())
{

}

void ClientProtoTesterRunner::Run() {
  tester_.SetService(svc_);
  tester_.SetProtoMessageFactory(*protfac_.get());

  tester_.Initiate([&]() {
    //syslog(_INFO) << "ClientProtoTester #ioended, stopping CcClient\n";
    //cccli.StopThreadsafe();
                   });

  // (tester_ does Accept...)

  //---

  Endpoint link_ep;
  Errcode err;
  tester_.GetAcceptEndpoint(link_ep, err);
  DCHECK(!err);

  cc_cli_ = make_unique<CcClient>(link_ep,
    svc_.CreateStreamConnectorFactory()->CreateStreamConnector(),
    svc_.CreateStreamFactory()->CreateStream(),
    make_shared<Strand>(tm_.DefIOC()),
    tester_.GetCommandDispatcher(),
    &tester_.GetEvents(),
    common_config::kMaxChunkBodySize,
    cc::CcClientBotOptions(BotId(), false));

  tester_.SetCcClient(*cc_cli_);

  cc_cli_->UseServerPingInterval();

  // We're about to send messages from proto sockets (imitating client/server as
  // one side) so we cannot use traffic encryption. Disable it.
  cc_cli_->DisableTrafficEncryption();
  cc_cli_->PrepareToStartNofail();

  cc_cli_->Start(RefTracker(CUR_LOC(), rtctx_.GetHandle(), []() {
    syslog(_INFO) << "cc client #ioended\n";
    }));



  //---

  tm_.Run();

  // we must have been stopped gracefully, without ioc.stop()
  DCHECK(0 == rtctx_.GetAtomicRefTrackerCount());

  // See Cleanup() comments.
  tester_.Cleanup();
}

}}
