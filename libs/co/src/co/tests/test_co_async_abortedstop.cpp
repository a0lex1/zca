#include "co/async/configs/thread_model_config_from_dict.h"
#include "co/async/stop_timer.h"
#include "co/async/tcp_service.h"
#include "co/async/client.h"
#include "co/async/server.h"
#include "co/net/endpoint.h"

#include "co/base/dict.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

#define llog() syslog(_INFO)

using namespace std;
using namespace co;
using namespace co::async;
using namespace co::async::configs;
using namespace boost::asio;
using namespace boost::posix_time;

namespace {
class SmellySession: public Session, public co::enable_shared_from_this<SmellySession> {
public:
  virtual ~SmellySession() = default;

  using Session::Session;

private:
  void BeginIo(RefTracker rt) override {
    rt.SetReferencedObject(shared_from_this());

    llog() << "ServerSession BeginIo\n";
    xserver_ = make_unique<Server>(
          co::net::TcpEndpoint("127.0.0.1", 0),
          make_shared<TcpService>(GetStream().GetIoContext()),
          gEmptySessionFactoryFunc);

    xserver_->PrepareToStartNofail();
    xserver_->Start(rt);

    GetStream().AsyncReadSome(mutable_buffers_1(&your_self_esteem_, 1),
      co::bind(&SmellySession::HandleReadSome, shared_from_this(), _1, rt));
  }
  void StopUnsafe() override {
    //DCHECK(xserver_);
    llog() << "ServerSession StopUnsafeIo\n";
    //xserver_->StopThreadsafe();
  }

  void HandleReadSome(Errcode err, RefTracker rt) {
    llog() << "1 byte has been read, stopping\n";
    StopThreadsafe();
  }
private:
  Uptr<Server> xserver_;
  char your_self_esteem_;
};
}

// run with test-repeat to detect memleaks
void test_co_async_abortedstop(TestInfo& ti) {

  size_t sleep_ms = 5;
  OverrideFromDict<string, string, size_t>(ti.opts_dict, "sleep", sleep_ms, ConsumeAction::kDontConsume);

  llog() << "sleep: " << sleep_ms << " msec\n";

  uint32_t startorder = 0;
  OverrideFromDict<string, string, uint32_t>(ti.opts_dict, "start-order", startorder, ConsumeAction::kConsume);
  DCHECK(startorder <= 2); // 0, 1, 2

  ThreadModelConfigFromDict tmconf(ThreadModelConfig(), ti.opts_dict, ConsumeAction::kDontConsume);
  ThreadModel tm(tmconf);

  auto service(make_shared<TcpService>(tm.DefIOC()));

  ServerWithSessList server(co::net::TcpEndpoint("127.0.0.1", 0), service,
                            gEmptySessionFactoryFunc);

  server.PrepareToStartNofail();

  Client client(server.GetLocalAddressToConnect(),
                service->CreateStreamConnectorFactory()->CreateStreamConnector(),
                make_shared<SmellySession>(service->CreateStreamFactory()->CreateStream(),
                                           make_shared<Strand>(tm.DefIOC())));

  StopTimer stoptimer(tm.DefIOC(), make_shared<Strand>(tm.DefIOC()),
                      tm, // target object to stop
                      milliseconds(sleep_ms));


  client.PrepareToStartNofail();
  stoptimer.PrepareToStartNofail();


  auto stopall =
      [&] () {
#if 0
    server.StopThreadsafe();
    client.StopThreadsafe();
    stoptimer.StopThreadsafe();
#endif
  };
  // Start phase
  RefTrackerContext rtctx(CUR_LOC());
  RefTracker rt_all(CUR_LOC(), rtctx.GetHandle(),
    [&] () {
  llog() << "; rt_all\n";
    });
  RefTracker rt_server(CUR_LOC(),
    [&] () {
  llog() << "; rt_server\n";
  stopall();
    }, rt_all);
  RefTracker rt_client(CUR_LOC(),
    [&] () {
  llog() << "; rt_client\n";
  stopall();
    }, rt_all);
  RefTracker rt_stoptimer(CUR_LOC(),
    [&] () {
  llog() << "; rt_stoptimer\n";
  stopall();
    }, rt_all);

  llog() << "Starting objects (startorder: " << startorder << ")\n";
  switch (startorder) {
  case 0:
    server.Start(rt_server);
    client.Start(rt_client);
    stoptimer.Start(rt_stoptimer);
    break;
  case 1:
    server.Start(rt_server);
    client.Start(rt_client);
    stoptimer.Start(rt_stoptimer);
    break;
  case 2:
    server.Start(rt_server);
    client.Start(rt_client);
    stoptimer.Start(rt_stoptimer);
    break;
  default: NOTREACHED();
  }
  rt_all = rt_server = rt_client = rt_stoptimer = RefTracker();

  llog() << "Doing tm.Run() ...\n";

  tm.Run();

  if (rtctx.GetAtomicRefTrackerCount() != 0) {
    rtctx._DbgLogPrintTrackedRefTrackers();

    llog() << "Cleaning up your abortion\n";

    rtctx.DisableOnReleaseCalls();

    server.CleanupAbortedStop();
    client.CleanupAbortedStop();
    stoptimer.CleanupAbortedStop();
  }

  llog() << "tm.Run() returned\n";
}




