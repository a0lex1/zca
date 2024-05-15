#include "zfront/daemonize.h"

#include "zca/modules/basecmd/front/basecmd_frontend_module.h"

#include "zca/frontend.h"

#include "co/async/capsule/capsule_builder.h"
#include "co/async/configs/thread_model_config_from_dict.h"
#include "co/net/default_endpoint_scheme_registry.h"

#include "co/base/cmdline/keyed_cmdline.h"

#include "co/xlog/configs.h"
#include "co/xlog/xlog.h"

#include <boost/asio/signal_set.hpp>

#include <thread>

using namespace std;
using namespace boost::asio;
using namespace co;
using namespace co::xlog::configs;
using namespace co::cmdline;
using namespace co::async;
using namespace co::async::capsule;
using namespace co::async::configs;
using namespace co::net;

class ZfrontObject : public LoopObjectNoreset {
public:
  virtual ~ZfrontObject() = default;

  ZfrontObject(ThreadModel& tm, const FrontendConfig& front_conf)
    :
    tm_(tm), front_conf_(front_conf)
  {
  }

  // [LoopObject::LoopObjectBase impl]
  void PrepareToStart(Errcode& err) override {
    frontend_ = make_unique<Frontend>(tm_, front_conf_, FrontendSeparationConfig());
    frontend_->AddModule(make_unique<modules::basecmd::front::BasecmdFrontendModule>());
    frontend_->PrepareToStart(err);
    if (err) {
      return;
    }
    signal_set_ = make_unique<signal_set>(tm_.DefIOC(),
      SIGINT, SIGTERM /*, SIGQUIT*/);
  }

  void CleanupAbortedStop() override {
    frontend_->CleanupAbortedStop();
    signal_set_->cancel();
  }

  void Start(RefTracker rt) override {
    signal_set_->async_wait([&, rt](Errcode err, int sig) {
      if (err) {
        syslog(_INFO) << "ERROR: Mo more waiting for signals (" << err << ")\n";
        return;
      }
      syslog(_INFO) << "WARNING: Signal " << sig << " caught. Stopping...\n";

      StopThreadsafe();
      });

    frontend_->Start(rt);
  }
  void StopThreadsafe() override {
    frontend_->StopThreadsafe();
    signal_set_->cancel();
  }

private:
  ThreadModel& tm_;
  FrontendConfig front_conf_;
  Uptr<Frontend> frontend_;
  Uptr<boost::asio::signal_set> signal_set_;
};


// back-bot-addr
// front-admin-addr
int zfront_main(int argc, char* argv[]) {
  KeyedCmdLine<char> cl(argc, argv);
  int ret{ 0 };

  LogConfigFromDict log_conf;
  ThreadModelConfigFromDict tm_conf;
  string back_bot_addr, front_admin_addr;
  Endpoint back_bot_ep, front_admin_ep;

  tm_conf.num_threads_default = std::thread::hardware_concurrency() * 2;

  GetDefaultEpSchemeRegistry().RegisterScheme("tcp", []() { return TcpEndpoint(); });

  try {
    InitLogWithConfig(log_conf);

    log_conf = LogConfigFromDict(LogConfig(), cl.GetNamedArgs(), ConsumeAction::kConsume);
    OverrideFromDict<string, string, string>(cl.GetNamedArgs(), "back-bot-addr", back_bot_addr, ConsumeAction::kConsume, Necessity::kRequired);
    OverrideFromDict<string, string, string>(cl.GetNamedArgs(), "front-admin-addr", front_admin_addr, ConsumeAction::kConsume, Necessity::kRequired);

    back_bot_ep = GetDefaultEpSchemeRegistry().CreateEndpointForURI(back_bot_addr);
    front_admin_ep = GetDefaultEpSchemeRegistry().CreateEndpointForURI(front_admin_addr);
  }
  catch (DictException& e) {
    cout << "Dict exception: " << e.what() << "\n";
    return -1;
  }
  catch (LogConfigException& e) {
    cout << "ConfigSet exception: " << e.what() << "\n";
    return -1;
  }

  syslog(_INFO) << "Using " << tm_conf.num_threads_default << " threads\n";
  syslog(_INFO) << "Backend CC addr:    " << back_bot_ep.ToString() << "\n";
  syslog(_INFO) << "Frontend admin addr:    " << front_admin_ep.ToString() << "\n";

  FrontendConfig front_conf;
  front_conf.bk_addr = back_bot_ep;
  front_conf.frontend_server_locaddr = front_admin_ep;

  static const size_t kMaxNormalExitIters = 2;
  static const size_t kMaxTotalIters = LoopOptions::kUnlimitedIterations; // default

#ifndef _WIN32
  syslog(_INFO) << "Daemonizing ...\n";
  Daemonize();
#endif

  CapsuleBuilder builder;
  builder
    .SetLoopOptionsCreator([]() {
    return make_unique<LoopOptions>(kMaxTotalIters,
    kMaxNormalExitIters);
      })
    .SetIoInitiatorCreator([&](ThreadModel& _tm_) {
        auto obj(make_unique<ZfrontObject>(_tm_, front_conf));
        return obj;
      })
    .SetThreadModelCreator([&]() {
        return make_unique<ThreadModel>(tm_conf);
      });

  Capsule capsule(builder.GetObjectFactory());

  // link it and run
  capsule.Initialize();
  capsule.Run();

  if (!cl.GetNamedArgs().empty()) {
    syslog(_INFO) << "P.S.: Unknown args left:\n";
    KeyedCmdLine<char>::PrintUnknownArgsLeft(cout, cl.GetNamedArgs(), "\t");
    return -3;
  }

  return ret;
}


int main(int argc, char* argv[]) {
  return zfront_main(argc, argv);
}

