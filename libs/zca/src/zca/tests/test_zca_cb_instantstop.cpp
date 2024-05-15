#include "zca/backend.h"
#include "zca/agent.h"

#include "co/async/configs/thread_model_config_from_dict.h"
#include "co/async/tcp_service.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

#include <boost/asio/deadline_timer.hpp>

using namespace std;
using namespace co;
using namespace co::async;
using namespace co::async::configs;
using namespace boost::asio;
using namespace boost::posix_time;

#define slog() syslog(_INFO) << "test_zca_cb_instantstop_*: "

void test_zca_cb_instantstop_serveronly(TestInfo& ti) {
  ThreadModel tm;
  Server server(co::net::TcpEndpoint("127.0.0.1", 0),
                make_shared<TcpService>(tm.DefIOC()),
                gEmptySessionFactoryFunc);

  slog() << "PrepareToStartNofail...\n";
  server.PrepareToStartNofail();

  RefTrackerContext rtctx(CUR_LOC());

  slog() << "server.Start()+StopThreadsafe()...\n";
  server.Start(RefTracker(CUR_LOC(), rtctx.GetHandle(),
                          [] () {

  }));
  server.StopThreadsafe();
  slog() << "tm.Run()...\n";
  tm.Run();
  slog() << "tm.Run() returned\n";

  rtctx._DbgLogPrintTrackedRefTrackers();
  DCHECK(!rtctx.GetAtomicRefTrackerCount());
}

void test_zca_cb_instantstop_fakebackend(TestInfo& ti) {

  bool start_together = false;
  OverrideFromDict<string, string, bool>(ti.opts_dict, "start-together", start_together, ConsumeAction::kDontConsume);

  ThreadModelConfigFromDict tmconf(ThreadModelConfig(), ti.opts_dict, ConsumeAction::kDontConsume);
  ThreadModel tm(tmconf);

  co::net::Endpoint back_remaddr = co::net::TcpEndpoint("127.0.0.1", 0);

  // Instead of Backend, use Server
  Server server(back_remaddr, make_shared<TcpService>(tm.DefIOC()),
                gEmptySessionFactoryFunc);

  server.PrepareToStartNofail();

  AgentConfig agent_conf(server.GetLocalAddressToConnect());

  Agent agent(tm, agent_conf, AgentSeparationConfig());

  RefTrackerContext rtctx(CUR_LOC());
  RefTracker rt_all(CUR_LOC(), rtctx.GetHandle(),
                []() {
           slog() << "; all #ioended\n";
                 });

  slog() << "* Starting backend, agent, Stopping backend (start_together=" << boolalpha << start_together << ")\n";

  agent.PrepareToStartNofail();

  if (start_together) {
    // start, (agent start), stop
    server.Start(rt_all);
    agent.Start(rt_all);
    server.StopThreadsafe();
  }
  else {
    // start, stop (agent start)
    server.Start(rt_all);
    server.StopThreadsafe();
    agent.Start(rt_all);
  }
  //agent.StopThreadsafe();

  rt_all = RefTracker(); // drop

  slog() << "* doing tm.Run()\n";
  tm.Run();
  slog() << "* tm.Run() returned\n";

  rtctx._DbgLogPrintTrackedRefTrackers();
  DCHECK(!rtctx.GetAtomicRefTrackerCount());

  slog() << "returning\n";
}



void test_zca_cb_instantstop(TestInfo& ti) {

  bool start_together = false;
  OverrideFromDict<string, string, bool>(ti.opts_dict, "start-together", start_together, ConsumeAction::kDontConsume);

  ThreadModelConfigFromDict tmconf(ThreadModelConfig(), ti.opts_dict, ConsumeAction::kDontConsume);
  ThreadModel tm(tmconf);

  BackendConfig backend_conf;
  
  backend_conf.admin_server_locaddr = co::net::TcpEndpoint("127.0.0.1", 0);
  backend_conf.cc_server_locaddr = co::net::TcpEndpoint("127.0.0.1", 0);
  Backend backend(tm, backend_conf, BackendSeparationConfig());

  backend.PrepareToStartNofail();

  AgentConfig agent_conf(backend.GetLocalAddressToConnect());

  Agent agent(tm, agent_conf, AgentSeparationConfig());

  RefTrackerContext rtctx(CUR_LOC());
  RefTracker rt_all(CUR_LOC(), rtctx.GetHandle(),
                []() {
           slog() << "; all #ioended\n";
                 });

  slog() << "* Starting backend, agent, Stopping backend (start_together=" << boolalpha << start_together << ")\n";

  agent.PrepareToStartNofail();

  if (start_together) {
    // start, (agent start), stop
    backend.Start(rt_all);
    agent.Start(rt_all);
    backend.StopThreadsafe();
  }
  else {
    // start, stop (agent start)
    backend.Start(rt_all);
    backend.StopThreadsafe();
    agent.Start(rt_all);
  }
  //agent.StopThreadsafe();

  rt_all = RefTracker(); // drop

  slog() << "* doing tm.Run()\n";
  tm.Run();
  slog() << "* tm.Run() returned\n";

  rtctx._DbgLogPrintTrackedRefTrackers();
  DCHECK(!rtctx.GetAtomicRefTrackerCount());

  slog() << "returning\n";
}

void test_zca_cb_instantstop_stopioc(TestInfo& ti) {
  ThreadModelConfigFromDict tmconf(ThreadModelConfig(), ti.opts_dict, ConsumeAction::kDontConsume);
  ThreadModel tm(tmconf);

  BackendConfig backend_conf;
  backend_conf.admin_server_locaddr = co::net::TcpEndpoint("127.0.0.1", 0);
  backend_conf.cc_server_locaddr = co::net::TcpEndpoint("127.0.0.1", 0);
  
  Backend backend(tm, backend_conf, BackendSeparationConfig());
  backend.PrepareToStartNofail();

  AgentConfig agent_conf(backend.GetLocalAddressToConnect());

  Agent agent(tm, agent_conf, AgentSeparationConfig());

  RefTrackerContext rtctx(CUR_LOC());
  RefTracker rt_all(CUR_LOC(), rtctx.GetHandle(),
                []() {
           slog() << "; all #ioended\n";
                 });

  slog() << "* Starting backend, agent\n";

  agent.PrepareToStartNofail();

  backend.Start(rt_all);
  agent.Start(rt_all);

  rt_all = RefTracker(); // drop

  deadline_timer timer(tm.DefIOC(), milliseconds(25));
  timer.async_wait([&tm] (Errcode e) {
    DCHECK(!e);
    slog() << "timer fired, stopping thread model\n";
    tm.StopThreadsafe();
  });

  slog() << "* doing tm.Run()\n";
  tm.Run();
  slog() << "* tm.Run() returned\n";

  slog() << rtctx.GetAtomicRefTrackerCount() << " reftrackers left, must be > 0\n";
  DCHECK(0 != rtctx.GetAtomicRefTrackerCount());

  slog() << "Cleaning up aborted stop\n";
  rtctx.DisableOnReleaseCalls();
  backend.CleanupAbortedStop();
  agent.CleanupAbortedStop();

  slog() << "returning\n";
}


