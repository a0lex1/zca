#include "cc/cc_client.h"
#include "cc/cc_server.h"
#include "cc/cc_server_events_from_func.h"
#include "cc/cc_client_events_from_func.h"
#include "cc/cc_client_cmd_disp_from_func.h"

#include "co/async/thread_model.h"
#include "co/async/configs/thread_model_config_from_dict.h"
#include "co/async/tcp.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;
using namespace co::async::configs;
using namespace cc;
using namespace boost::posix_time;
using co::net::Endpoint;
using co::net::TcpEndpoint;

void test_cc_client_server_zombie_kill(TestInfo& test_info) {
  bool stop_ioc_instead = false;
  OverrideFromDict<string, string, bool>(test_info.opts_dict, "stop-ioc", stop_ioc_instead, ConsumeAction::kDontConsume);

  ThreadModelConfigFromDict tmconf(ThreadModelConfig(), test_info.opts_dict, ConsumeAction::kDontConsume);
  ThreadModel tm(tmconf);
  io_context& ioc(tm.AcquireIoContextForThreadGroup(0));
  RefTrackerContext rtctx(CUR_LOC());

  CcServerEventsFromFunc server_events;
  CcServer server(
    TcpEndpoint("127.0.0.1", 0),
    ServerObjects(make_shared<TcpStreamFactory>(ioc),
                  make_unique<TcpStreamAcceptor>(ioc),
                  ioc),
    &server_events,
    64*1024);

  // A timer that helps us to detect errors
  boost::asio::deadline_timer timer(tm.DefIOC());

  /* -----------------------------------------------------------------------
    Set ping interval for server
    Server should kill client which uses ping interval = 10
  ------------------------------------------------------------------------ */
  server.SetPingInterval(boost::posix_time::seconds(2)); // Client uses 10 which is greater
  server.SetTrafficEncryptionKeys("appl3", 5, "b4n4n4", 6);
  
  server_events.SetOnBotHandshakeComplete([](Shptr<ICcBot> bot) {
    syslog(_INFO) << "* SERVER-SIDE OnBotHandshakeComplete\n";
    // RUNNING INSIDE BOT FIBER
    // Handshake complete. Just test something here.
    cc::ICcBotReadonlyData& rd_data(bot->GetReadonlyData());
    DCHECK(nullptr != rd_data.GetHandshakeData());
    syslog(_INFO) << "hshake_data = {botid=" << rd_data.GetHandshakeData()->GetBotId().ToStringRepr() << "}\n";
                              });

  // OnBotRemoved, called because the server will kill client whose ping interval is a lot greater
  auto checkNoBotsStopServer = [&](ICcBotList& bl) {
    // RUNNING INSIDE BOTLIST FIBER
    syslog(_INFO) << "server-side CHECK 2, bot count : " << bl.GetCount() << " ***\n";
    DCHECK(bl.GetCount() == 0);
    DCHECK(bl.begin() == bl.end());

    // Now we can stop the server

    if (stop_ioc_instead) {

      tm.StopThreadsafe();
      rtctx.DisableOnReleaseCalls();
      server.CleanupAbortedStop();
    }
    else {
      server.StopThreadsafe();
      timer.cancel();
    }
  };
  server_events.SetOnBotRemoved([&](Shptr<ICcBot> bot) {
    syslog(_INFO) << "* SERVER-SIDE OnBotRemoved\n";
    cc::ICcBotReadonlyData& rd_data(bot->GetReadonlyData());
    DCHECK(nullptr != rd_data.GetHandshakeData());
    syslog(_INFO) << "- bot removed (botid=" << rd_data.GetHandshakeData()->GetBotId().ToStringRepr() << "}\n";
    // Verify there is no bots on server now
    server.ExecuteBotListAccessCallback(checkNoBotsStopServer);
                              });

  CcClientEventsFromFunc client_events;
  CcClientCmdDispFromFunc cli_cmd_disp;
  cli_cmd_disp.SetCommandHandler([](Uptr<std::string> cmd_opaque_data,
                                 std::string& cmd_result_opaque_data,
                                 EmptyHandler handler)
                                 {
                                 });

  static const bool kPostponeEnable = true;
  static const time_duration kHandshakePostponeDelay = milliseconds(0);

  CcClientBotOptions bot_opts(BotId(), kPostponeEnable, kHandshakePostponeDelay);

  // ---

  server.PrepareToStartNofail();
  auto locaddr(server.GetLocalAddressToConnect());
    
  CcClient client(locaddr,
                  make_shared<TcpStreamConnector>(),
                  make_unique<TcpStream>(ioc),
                  make_shared<Strand>(ioc), // ioc_for_strand
                  cli_cmd_disp,
                  &client_events,
                  100500, // max_chunk_body_size
                  bot_opts);

  /* -----------------------------------------------------------------------
    Set ping interval for bot
    Server should kill it as it is a lot greater than server's ping interval
  ------------------------------------------------------------------------ */
  client.SetPingInterval(boost::posix_time::seconds(10));
  client.SetTrafficEncryptionKeys("b4n4n4", 6, "appl3", 5);

  client_events.SetOnHandshakeReplyReceived([&]() {
    syslog(_INFO) << "* CLIENT-SIDE OnHandshakeReplyReceived\n";
                                            });

  // ---

  // Add timer with 5 seconds
  timer.expires_from_now(boost::posix_time::seconds(5));
  timer.async_wait([] (Errcode err) {
    // If no error, then the test has failed (the bot wasn't killed by timeout)
    if (!err) {
      NOTREACHED();
    }
  });

  // ---

  bool server_stopped = false, client_stopped = false, all_stopped = false;
  {
    co::RefTracker rt_all(CUR_LOC(),
      rtctx.GetHandle(),
      [&]() {
        all_stopped = true;
        syslog(_INFO) << "All CC stopped\n";
      }
    );
    co::RefTracker rt_server(CUR_LOC(),
      [&]() {
        server_stopped = true;
        syslog(_INFO) << "CC server stopped\n";
      },
      rt_all
    );
    co::RefTracker rt_client(CUR_LOC(),
      [&]() {
        client_stopped = true;
        //DCHECK(err == CcErrc::bot_killed);
        syslog(_INFO) << "CC client stopped\n";
      },
      rt_all
    );

    client.PrepareToStartNofail();

    server.Start(rt_server);
    syslog(_INFO) << "CC server started\n";

    client.Start(rt_client);
    syslog(_INFO) << "CC Client started\n";
  }
  tm.Run();
  syslog(_INFO) << "ThreadModel::Run() returned\n";

  if (stop_ioc_instead) {
    /* */
  }
  else {
    DCHECK(server_stopped);
    DCHECK(client_stopped);
    DCHECK(all_stopped);
  }

  //
}

