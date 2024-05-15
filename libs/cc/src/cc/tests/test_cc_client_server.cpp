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

/**************************************************************
* 
* Scheme:
*
* server_events.OnBotRemoved{
*   server.ExecuteBotListAccessCallback{
*     DCHECK(0bots);
*   }
* }
* client_events.OnHandshakeReplyReceived{
*   server.ExecuteBotListAccessCallback{
*     DCHECK(1bots);
*     client.Stop();
*   }
* }
* client.Start();
* server.Start();
*
**************************************************************/
void test_cc_client_server_botlist_access(TestInfo& test_info) {
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

  //server.DisablePinging();
  server.SetPingInterval(boost::posix_time::milliseconds(10));
  server.SetTrafficEncryptionKeys("appl3", 5, "b4n4n4", 6);
  
  server_events.SetOnBotHandshakeComplete([](Shptr<ICcBot> bot) {
    syslog(_INFO) << "* SERVER-SIDE OnBotHandshakeComplete\n";
    // RUNNING INSIDE BOT FIBER
    // Handshake complete. Just test something here.
    cc::ICcBotReadonlyData& rd_data(bot->GetReadonlyData());
    DCHECK(nullptr != rd_data.GetHandshakeData());
    syslog(_INFO) << "hshake_data = {botid=" << rd_data.GetHandshakeData()->GetBotId().ToStringRepr() << "}\n";
                              });

  // OnBotRemoved
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
      //server.StopSessions(); // obsolete, remove
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

  //client.DisablePinging();
  client.SetPingInterval(boost::posix_time::seconds(1));
  client.SetTrafficEncryptionKeys("b4n4n4", 6, "appl3", 5);

  // On handshake reply received, verify 1bots and stop client
  auto checkBotStopClient = [=, &client](ICcBotList& bl) {
    // inside server's acceptor fiber (called from ExecuteBotListAccessCallback())
    syslog(_INFO) << "server-side CHECK 1, bot count : " << bl.GetCount() << " ***\n";
    // Verify the bot is on server
    DCHECK(bl.GetCount() == 1);
    CcBotRecordIterator rec_it(bl.begin());
    Shptr<ICcBot> xx = *rec_it;
    xx->GetReadonlyData();
    xx->GetReadonlyData().GetHandshakeData();

    syslog(_INFO) << "Stopping client and leaving\n";
    client.StopThreadsafe();
  };

  client_events.SetOnHandshakeReplyReceived([&]() {
    syslog(_INFO) << "* CLIENT-SIDE OnHandshakeReplyReceived\n";
 
    server.ExecuteBotListAccessCallback(checkBotStopClient);
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





















