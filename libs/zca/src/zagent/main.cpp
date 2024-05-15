#include "zca/agent.h"
#include "zca/configs/agent_config_from_dict.h"
#include "zca/modules/dummy/dummy_module.h"
#include "zca/modules/basecmd/ag/basecmd_agent_module.h"

#include "cc/make_random_bot_id.h"

#include "co/net/default_endpoint_scheme_registry.h"
#include "co/async/configs/thread_model_config_from_dict.h"
#include "co/base/cmdline/keyed_cmdline.h"

#include "co/xlog/xlog.h"
#include "co/xlog/configs/log_config_from_dict.h"
#include "co/xlog/configs/init_log_with_config.h"

#include "co/common_config.h"

#include <boost/asio/signal_set.hpp>

using namespace std;
using namespace boost::asio;
using namespace co;
using namespace co::cmdline;
using namespace co::async;
using namespace co::async::configs;
using namespace co::xlog::configs;
using namespace co::net;

int zagent_main(int argc, char* argv[]) {
  KeyedCmdLine<char> cl(argc, argv);
  RefTrackerContext rtctx(CUR_LOC());
  RandGen randgen(GenerateSeed());

  GetDefaultEpSchemeRegistry().RegisterScheme("tcp", []() { return TcpEndpoint(); });

  try {
    InitLogWithConfig(LogConfigFromDict(LogConfig(), cl.GetNamedArgs(), ConsumeAction::kConsume));
    syslog(_INFO) << "Log initialized\n";

    syslog(_INFO) << "Parsing parameters\n";
    ThreadModelConfigFromDict tm_conf(ThreadModelConfig(), cl.GetNamedArgs(), ConsumeAction::kConsume);
    ThreadModel tm(tm_conf);

    std::string remuri;
    OverrideFromDict<string, string, string>(cl.GetNamedArgs(),
                                             "remuri", 
                                             remuri,
                                             ConsumeAction::kConsume,
                                             Necessity::kRequired);
    syslog(_INFO) << "Initializing configs\n";
    string scheme;
    Endpoint remaddr;
    remaddr = GetDefaultEpSchemeRegistry().CreateEndpointForURI(remuri, &scheme);
    syslog(_INFO) << "Remote address      : [" << scheme << "://] " << remaddr.ToString() << "\n";
    syslog(_INFO) << "Num threads default : " << tm.GetNumThreadsDefault() << "\n";

    // ping_interval -> 0, use server's interval
    AgentConfig agent_conf(remaddr, boost::posix_time::milliseconds(0), co::common_config::kMaxChunkBodySize);
    if (cl.HasNamedArg("bid")) {
      if (!agent_conf.bot_id.FromStringRepr(cl.GetNamedArgs()["bid"])) {
        cout << "Cannot parse bid\n";
        return -1;
      }
    }
    else {
      agent_conf.bot_id = cc::MakeRandomBotId(randgen);

    }
    AgentSeparationConfig agent_sep_conf;
    Agent agent(tm, agent_conf, agent_sep_conf);

    syslog(_INFO) << "Adding modules\n";
    agent.AddModule(make_unique<modules::basecmd::ag::BasecmdAgentModule>());
    agent.AddModule(make_unique<modules::dummy::DummyAgentModule>());

    signal_set sigset(tm.DefIOC(), SIGINT, SIGTERM /*, SIGQUIT*/);
    sigset.async_wait([&](Errcode err, int sig) {
      if (err) {
        syslog(_ERR) << "Mo more waiting for signals (" << err << ")\n";
        return;
      }
      syslog(_WARN) << "Signal " << sig << " caught. Stopping agent...\n";
      agent.StopThreadsafe();
      });

    agent.PrepareToStartNofail();

    syslog(_INFO) << "Starting agent...\n";
    agent.Start(RefTracker(CUR_LOC(), rtctx.GetHandle(), [&]() {
      syslog(_INFO) << "Agent stopped\n";
      sigset.cancel();
      }));

    syslog(_INFO) << "Running thread model...\n";
    tm.Run();

    syslog(_INFO) << "Returned from ThreadModel::Run()\n";
  }
  catch (DictException& e) {
    cout << "Dict exception: " << e.what() << "\n";
    return -1;
  }
  catch (ConfigException& e) {
    cout << "Config exception: " << e.what() << "\n";
    return -1;
  }
  catch (boost::system::system_error& e) {
    cout << "boost::system::system_error exception: " << e.what() << "\n";
  }
  catch (std::system_error& e) {
    cout << "std::system_error exception: " << e.what() << "\n";
  }
  if (!cl.GetNamedArgs().empty()) {
    cout << "Unknown args:\n";
    KeyedCmdLine<char>::PrintUnknownArgsLeft(cout, cl.GetNamedArgs(), "\t");
  }

  return 0;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
  return zagent_main(argc, argv);
}

