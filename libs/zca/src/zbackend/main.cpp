#include "zca/backend.h"
#include "zca/configs/backend_config_from_dict.h"
#include "zca/modules/dummy/dummy_module.h"
#include "zca/modules/basecmd/back/basecmd_backend_module.h"

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

int zbackend_main(int argc, char* argv[]) {
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

    std::string adminuri, ccuri;
    OverrideFromDict<string, string, string>(cl.GetNamedArgs(), "adminuri", adminuri, ConsumeAction::kConsume, Necessity::kRequired);
    OverrideFromDict<string, string, string>(cl.GetNamedArgs(), "ccuri",  ccuri, ConsumeAction::kConsume, Necessity::kRequired);
    syslog(_INFO) << "Initializing configs\n";
    string scheme;
    Endpoint adminaddr, ccaddr;
    adminaddr = GetDefaultEpSchemeRegistry().CreateEndpointForURI(adminuri, &scheme);
    ccaddr = GetDefaultEpSchemeRegistry().CreateEndpointForURI(ccuri, &scheme);
    syslog(_INFO) << "Local admin address  : [" << scheme << "://] " << adminaddr.ToString() << "\n";
    syslog(_INFO) << "Local bot address    : [" << scheme << "://] " << ccaddr.ToString() << "\n";
    syslog(_INFO) << "Num threads default  : " << tm.GetNumThreadsDefault() << "\n";

    BackendConfig backend_conf(adminaddr, ccaddr);
    BackendSeparationConfig backend_sep_conf;
    Backend backend(tm, backend_conf, backend_sep_conf);

    syslog(_INFO) << "Adding modules\n";
    backend.AddModule(make_unique<modules::basecmd::back::BasecmdBackendModule>());

    signal_set sigset(tm.DefIOC(), SIGINT, SIGTERM /*, SIGQUIT*/);
    sigset.async_wait([&](Errcode err, int sig) {
      if (err) {
        syslog(_ERR) << "Mo more waiting for signals (" << err << ")\n";
        return;
      }
      syslog(_WARN) << "Signal " << sig << " caught. Stopping backend...\n";
      backend.StopThreadsafe();
      });

    backend.PrepareToStartNofail();

    syslog(_INFO) << "Starting backend...\n";
    backend.Start(RefTracker(CUR_LOC(), rtctx.GetHandle(), [&]() {
      syslog(_INFO) << "Backend stopped\n";
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
  return zbackend_main(argc, argv);
}

