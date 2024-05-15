#include "./dbserver_stress_test.h"

#include "co/async/test_kit/test_session.h"
#include "co/async/server.h"
#include "co/async/session_park.h"
#include "co/async/tcp.h"
#include "co/base/strings.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace async;
using namespace co::async::test_kit;
using co::net::Endpoint;
using co::net::TcpEndpoint;
using milliseconds = boost::posix_time::milliseconds;
using time_duration = boost::posix_time::time_duration;

namespace co {
namespace async {
namespace test_kit {

DbserverStressTest::DbserverStressTest(const ThreadModelConfig& tm_conf, const DbserverTestParams& par)
  :
  tm_conf_(make_unique<ThreadModelConfig>(tm_conf)),
  tm_(nullptr),
  par_(par)
{
}

DbserverStressTest::DbserverStressTest(ThreadModel& tm, const DbserverTestParams& par)
  : tm_conf_(nullptr), tm_(&tm), par_(par)
{

}

void DbserverStressTest::DoStressTest() {
  Uptr<ThreadModel> mt;
  ThreadModel* ttm;
  if (tm_ != nullptr) {
    ttm = tm_;
  }
  else {
    DCHECK(tm_conf_);
    mt = make_unique<ThreadModel>(*tm_conf_.get());
    ttm = mt.get();
  }
  ThreadModel& tm(*ttm);

  
  ServerWithSessList server(
    TcpEndpoint("127.0.0.1", 0),
    ServerObjects(
      make_shared<TcpStreamFactory>(tm.AcquireIoContextForThreadGroup(par_.threadgrp_srv_sessions)),
      make_unique<TcpStreamAcceptor>(tm.AcquireIoContextForThreadGroup(par_.threadgrp_srv_acceptor)),
      tm.AcquireIoContextForThreadGroup(par_.threadgrp_srv_sessions)),
    [](Uptr<Stream> new_stm, Shptr<Strand> strand) {
      auto s = make_shared<TestServerSession>(move(new_stm),
                                            strand,
                                            TestSessionParams::ReadForever());
      SET_DEBUG_TAG(*s.get(), string_printf("srvsess").c_str());
      return s;
    }
    );

  server.PrepareToStartNofail();
  syslog(_INFO) << "[*] Server is bound to " << server.GetLocalAddressToConnect().ToString() << "\n";

  SessionPark client_park_conflooders(
    par_.num_conflood_sessions,
    par_.restart_conflood_sessions,
    server.GetLocalAddressToConnect(),
    [](Uptr<Stream> new_stm, size_t slot) {
      auto s = make_shared<TestClientSession>(std::move(new_stm),
                                            make_shared<Strand>(new_stm->GetIoContext()), //-V522
                                            TestSessionParams::ConFlood());
      SET_DEBUG_TAG(*s.get(), string_printf("confloodsess_slot%d", slot).c_str());
      return s;
    },
    make_shared<TcpStreamFactory>(tm.AcquireIoContextForThreadGroup(par_.threadgrp_cli_conflood_sessions)),
    make_unique<TcpStreamConnector>());

  // ---

  auto get_portion_count = [&](size_t slot) {
    return (slot)*par_.write_count_multipl;
  };
  auto get_portion_size = [&](size_t slot) {
    return (slot)*par_.write_size_multipl;
  };
  auto get_write_delay = [&](size_t slot) {
    return milliseconds((slot)*par_.write_delay_multipl);
  };

  SessionPark client_park_writeflooders(
    par_.num_writer_sessions,
    par_.restart_writer_sessions,
    server.GetLocalAddressToConnect(),
    [&](Uptr<Stream> new_stm, size_t slot) {
      size_t port_size = get_portion_size(slot);
      size_t port_count = get_portion_count(slot);
      time_duration delay = get_write_delay(slot);
      syslog(_TRACE) << "Creating writer session#" << slot << " with size=" << port_size << ",count=" << port_count << ",delay=" << delay.total_milliseconds() << " msec\n";
      auto sess = make_shared<TestClientSession>(
        std::move(new_stm),
        make_shared<Strand>(new_stm->GetIoContext()), // used before move()! (prev line) //-V522
        TestSessionParams::Write(
          port_size,
          port_count,
          delay),
          static_cast<int>(slot) /* integer tag */);
      SET_DEBUG_TAG(*sess.get(), string_printf("wrfloodsess_slot%d", slot).c_str());
      return sess;
    },
    make_shared<TcpStreamFactory>(tm.AcquireIoContextForThreadGroup(par_.threadgrp_cli_writeflood_sessions)),
    make_shared<TcpStreamConnector>()
    );

  // ---

  auto rtctx = RefTrackerContext(CUR_LOC());

  bool server_stopped = false, cli_conflooders_stopped = false, cli_writeflooders_stopped = false,
    all_stopped = false;
  {
    RefTracker rt_all(CUR_LOC(),
                      rtctx.GetHandle(),
                      [&]() {
      syslog(_DBG) << "All stopped\n";
      all_stopped = true;
    });

    RefTracker rt_srv_stopped(CUR_LOC(), [&]() {
        // inside unknown fiber
        syslog(_DBG) << "Server stopped\n";
        server_stopped = true;
        // Must be stopped!
      },
      rt_all);

    RefTracker rt_cf_stopped(CUR_LOC(), [&]() {
        syslog(_DBG) << "ClientPark (connection flooders) stopped\n";
        cli_conflooders_stopped = true;
      },
      rt_all);

    RefTracker rt_wf_stopped(CUR_LOC(), [&]() {
        syslog(_DBG) << "ClientPark (write flooders) stopped\n";
        cli_writeflooders_stopped = true;
      },
      rt_all);

    server.Start(rt_srv_stopped);
    //syslog(_DBG) << "Server started\n";

    client_park_conflooders.Start(rt_cf_stopped);
    syslog(_DBG) << "ClientPark (connection flooders) started\n";

    client_park_writeflooders.Start(rt_wf_stopped);
    syslog(_DBG) << "ClientPark (write flooders) started\n";
  }

  // ---

  boost::asio::deadline_timer dt(tm.AcquireIoContextForThreadGroup(0));
  dt.expires_from_now(boost::posix_time::milliseconds(par_.server_stop_delay_ms));

  HandlerWithErrcode timeout_handler;
  if (par_.stop_ioc_instead_of_server) {
    timeout_handler = [&](Errcode err) {
      syslog(_INFO) << "Kill timer: fired, STOPPING THREAD MODEL\n";
      tm.StopThreadsafe();
    };
  }
  else {
    timeout_handler = [&](Errcode err) {
      // INSIDE UNKNOWN FIBER
      syslog(_INFO) << "Kill timer: fired, stopping PARKS and SERVER\n";
      DCHECK(!err);
      // initiate i/o stop and sell crack to teenagers

      syslog(_INFO) << "Kill timer: Stopping parks\n";

      //./stresstest_co_listserver stresstest_co_async_dbserver_u --conflooders=0 --writers=2 --wcm=1 --wsm=1 --wdm=5 --st^C
      // crash _dbg_dtor_
      client_park_conflooders.StopThreadsafe(); // i/o not ended yet
      client_park_writeflooders.StopThreadsafe();

      syslog(_INFO) << "Kill timer: Stopping server (&sessions)...\n";
      server.StopThreadsafe();

      // will exit tm.Run() when all i/os ended
    };
  }

  dt.async_wait(timeout_handler);

  tm.Run();

  syslog(_INFO) << "\n+!++ tmodel.Run() returned ++!+\n\n";

  if (par_.stop_ioc_instead_of_server) {
    syslog(_INFO) << "ioc-stop: Disabling OnRelease calls...\n";
    rtctx.DisableOnReleaseCalls();

    syslog(_INFO) << "ioc-sop: Doing server.CleanupAbortedStop()\n";
    server.CleanupAbortedStop();
  }
  else {
    DCHECK(server_stopped);
    rtctx._DbgLogPrintTrackedRefTrackers();
    DCHECK(!rtctx.GetAtomicRefTrackerCount());
    DCHECK(cli_conflooders_stopped);
    DCHECK(cli_writeflooders_stopped);
  }
}


}}}
