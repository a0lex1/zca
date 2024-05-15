#include "co/async/loop_object_park.h"
#include "co/async/server.h"
#include "co/async/client.h"
#include "co/async/tcp.h"
#include "co/async/test_kit.h"

#include "co/base/dict.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co::async::test_kit;
using namespace co;
using namespace co::async;
using co::net::Endpoint;
using co::net::TcpEndpoint;

// try with --test-repeat, can stuck
void test_co_async_sesslist_server_instantstop_multiclient(TestInfo& ti) {

  bool start_together = false;
  OverrideFromDict<string, string, bool>(ti.opts_dict, "start-together", start_together, ConsumeAction::kDontConsume);

  syslog(_DBG) << "start_together: " << boolalpha << start_together << "\n";

  ThreadModelConfig tmconf;
  ThreadModel tmodel(tmconf);
  io_context& ioc0(tmodel.AcquireIoContextForThreadGroup(0));

  ServerWithSessList server(
    TcpEndpoint("127.0.0.1", 0),
    ServerObjects(
      make_shared<TcpStreamFactory>(ioc0),
      make_unique<TcpStreamAcceptor>(ioc0),
      ioc0),
    [&ioc0](Uptr<Stream> new_stm, Shptr<Strand> session_strand) {
      return make_shared<TestServerSession>(std::move(new_stm),
                                            session_strand,
                                            TestSessionParams::ReadForever());
    });
  server.PrepareToStartNofail();

  Endpoint locaddr = server.GetLocalAddressToConnect();
  syslog(_DBG) << "server.GetLocalAddressToConnect()() = " << locaddr.ToString() << "\n";

  LoopObjectPark objpark(10,
                         false,
                         make_shared<Strand>(ioc0),
                         [&, locaddr] (size_t slot) {
    auto client = make_unique<Client>(
      locaddr,
      make_shared<TcpStreamConnector>(),
      make_shared<TestClientSession>(
                  make_unique<TcpStream>(ioc0),
                  make_shared<Strand>(ioc0),
                  TestSessionParams::WriteAndDisconnect()));
    return move(client);
  });

  RefTrackerContext rtctx(CUR_LOC());
  bool server_stopped = false, client_park_stopped = false, all_stopped = false;
  {
    co::RefTracker rt_all(CUR_LOC(), rtctx.GetHandle(),
      [&]() {
        syslog(_INFO) << "All stopped\n";
        all_stopped = true;
      }
    );
    co::RefTracker rt_server(CUR_LOC(),
      [&]() {
        syslog(_INFO) << "Server stopped\n";
        server_stopped = true;
      },
      rt_all
        );
    co::RefTracker rt_client_or_park(CUR_LOC(),
      [&]() {
      syslog(_INFO) << "ClientPark #ioended\n";
      client_park_stopped = true;
      //server.StopThreadsafe();
    }, rt_all );

    objpark.PrepareToStartNofail();

    if (start_together) {
      server.Start(rt_server);
      objpark.Start(rt_client_or_park);
      server.StopThreadsafe();
    }
    else {
      server.Start(rt_server);
      server.StopThreadsafe();
      objpark.Start(rt_client_or_park);
    }
  }

  tmodel.Run();
  syslog(_INFO) << "ThreadModel::Run() returned\n";

  syslog(_INFO) << "CC accepted connections: " << server._DbgNumAcceptedConnections() << "\n";

  rtctx._DbgLogPrintTrackedRefTrackers();
  DCHECK(0 == rtctx.GetAtomicRefTrackerCount());

  DCHECK(server_stopped);
  DCHECK(client_park_stopped);
  DCHECK(all_stopped);
}


// try with --test-repeat, can stuck
void test_co_async_sesslist_server_instantstop(TestInfo& info) {
  ThreadModelConfig tmconf;
  ThreadModel tmodel(tmconf);
  io_context& ioc0(tmodel.AcquireIoContextForThreadGroup(0));

  ServerWithSessList server(
    TcpEndpoint("127.0.0.1", 0),
    ServerObjects(
      make_shared<TcpStreamFactory>(ioc0),
      make_unique<TcpStreamAcceptor>(ioc0),
      ioc0),
    [&ioc0](Uptr<Stream> new_stm, Shptr<Strand> session_strand) {
      return make_shared<TestServerSession>(std::move(new_stm),
                                            session_strand,
                                            TestSessionParams::ReadForever());
    });
  server.PrepareToStartNofail();

  Endpoint locaddr = server.GetLocalAddressToConnect();
  syslog(_INFO) << "server.GetLocalAddressToConnect()() = " << locaddr.ToString() << "\n";
  Client client(
    locaddr,
    make_shared<TcpStreamConnector>(),
    make_shared<TestClientSession>(
                make_unique<TcpStream>(ioc0),
                make_shared<Strand>(ioc0),
                TestSessionParams::WriteAndDisconnect()));

  RefTrackerContext rtctx(CUR_LOC());
  bool server_stopped = false, client_stopped = false, all_stopped = false;
  {
    co::RefTracker rt_all(CUR_LOC(), rtctx.GetHandle(),
      [&]() {
        syslog(_INFO) << "All stopped\n";
        all_stopped = true;
      }
    );
    co::RefTracker rt_server(CUR_LOC(),
      [&]() {
        syslog(_INFO) << "Server stopped\n";
        server_stopped = true;
      },
      rt_all
        );
    co::RefTracker rt_client(CUR_LOC(),
      [&]() {
        syslog(_INFO) << "Client stopped\n";
        client_stopped = true;
        //server.StopThreadsafe();
      },
      rt_all
        );

    client.PrepareToStartNofail();

    server.Start(rt_server);
    client.Start(rt_client);
    server.StopThreadsafe();
  }

  tmodel.Run();

  rtctx._DbgLogPrintTrackedRefTrackers();
  DCHECK(0 == rtctx.GetAtomicRefTrackerCount());

  syslog(_INFO) << "ThreadModel::Run() returned\n";

  // It's possible to have client.GetConnectError()
  if (client.GetConnectError()) {
    syslog(_INFO) << "This time the client was stopped with connect error: " << client.GetConnectError() << "\n";
  }
  else {
    syslog(_INFO) << "This time the client was stopped without connect error\n";
  }
  DCHECK(server_stopped);
  DCHECK(client_stopped);
  DCHECK(all_stopped);
}

// Old

void test_co_async_sesslist_server(TestInfo& info) {
  ThreadModelConfig tmconf;
  ThreadModel tmodel(tmconf);
  io_context& ioc0(tmodel.AcquireIoContextForThreadGroup(0));

  ServerWithSessList server(
    TcpEndpoint("127.0.0.1", 0),
    ServerObjects(
      make_shared<TcpStreamFactory>(ioc0),
      make_unique<TcpStreamAcceptor>(ioc0),
      ioc0),
    [&ioc0](Uptr<Stream> new_stm, Shptr<Strand> session_strand) {
      return make_shared<TestServerSession>(std::move(new_stm),
                                            session_strand,
                                            TestSessionParams::ReadForever());
    });
  server.PrepareToStartNofail();

  Endpoint locaddr = server.GetLocalAddressToConnect();
  syslog(_INFO) << "server.GetLocalAddressToConnect()() = " << locaddr.ToString() << "\n";
  Client client(////////////////////////////////////////
    locaddr,
    make_shared<TcpStreamConnector>(),
    make_shared<TestClientSession>(
                make_unique<TcpStream>(ioc0),
                make_shared<Strand>(ioc0),
                TestSessionParams::WriteAndDisconnect()));

  RefTrackerContext rtctx(CUR_LOC());
  bool server_stopped = false, client_stopped = false, all_stopped = false;
  {
    co::RefTracker rt_all(CUR_LOC(), rtctx.GetHandle(),
      [&]() {
        syslog(_INFO) << "All stopped\n";
        all_stopped = true;
      }
    );
    co::RefTracker rt_server(CUR_LOC(),
      [&]() {
        syslog(_INFO) << "Server stopped\n";
        server_stopped = true;
      },
      rt_all
        );
    co::RefTracker rt_client(CUR_LOC(),
      [&]() {
        syslog(_INFO) << "Client stopped\n";
        client_stopped = true;
        server.StopThreadsafe();
        //server.StopSessions(); // obsolete, remove
      },
      rt_all
        );
    server.Start(rt_server);
    syslog(_INFO) << "Server started\n";

    client.PrepareToStartNofail();
    syslog(_INFO) << "Client PrepareToStart done\n";

    client.Start(rt_client);
    syslog(_INFO) << "Client started\n";
  }
  tmodel.Run();
  //server.UnsetupAcceptor();
  syslog(_INFO) << "ThreadModel::Run() returned\n";

  if (client.GetConnectError()) {
    syslog(_ERR) << "GetConnectError() = " << client.GetConnectError() << "\n";
  }
  DCHECK(!client.GetConnectError());
  DCHECK(server_stopped);
  DCHECK(client_stopped);
  DCHECK(all_stopped);

}








