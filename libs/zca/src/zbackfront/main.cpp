#include "zbackfront/daemonize.h"

#include "zca/modules/basecmd/back/basecmd_backend_module.h"
#include "zca/modules/basecmd/front/basecmd_frontend_module.h"
#include "zca/modules/basecmd/ag/basecmd_agent_module.h"

#include "zca/backend.h"
#include "zca/frontend.h"

#include "co/async/capsule/capsule_builder.h"
#include "co/async/configs/thread_model_config_from_dict.h"

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

class ZbackfrontObject : public LoopObjectNoreset {
public:
  virtual ~ZbackfrontObject() = default;

  ZbackfrontObject(ThreadModel& tm, const BackendConfig& back_conf,
    FrontendConfig& front_conf)
    :
    tm_(tm), back_conf_(back_conf), front_conf_(front_conf)
  {
  }

  // [LoopObject::LoopObjectBase impl]
  void PrepareToStart(Errcode& err) override {
    back_conf_.admin_server_locaddr = TcpEndpoint("127.0.0.1", 0);
    backend_ = make_unique<Backend>(tm_, back_conf_, BackendSeparationConfig());
    backend_->AddModule(make_unique<modules::basecmd::back::BasecmdBackendModule>());
    backend_->PrepareToStart(err);
    if (err) {
      return;
    }
    auto back_admin_addr(backend_->GetLocalAddressToConnect());

    front_conf_.bk_addr = back_admin_addr;
    frontend_ = make_unique<Frontend>(tm_, front_conf_, FrontendSeparationConfig());
    frontend_->AddModule(make_unique<modules::basecmd::front::BasecmdFrontendModule>());
    frontend_->PrepareToStart(err);
    if (err) {
      return;
    }
    auto back_cc_addr(backend_->GetLocalCcAddressToConnect());
    auto front_admin_addr(frontend_->GetLocalAddressToConnect());
    syslog(_INFO)
      << "Frontend admin addr: " << front_admin_addr.ToString() << "\n"
      << "Backend bot addr: " << back_cc_addr.ToString() << "\n";

    signal_set_ = make_unique<signal_set>(tm_.DefIOC(),
      SIGINT, SIGTERM /*, SIGQUIT*/);
  }

  void CleanupAbortedStop() override {
    backend_->CleanupAbortedStop();
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

    backend_->Start(rt);
    frontend_->Start(rt);
  }
  void StopThreadsafe() override {
    backend_->StopThreadsafe();
    frontend_->StopThreadsafe();
    signal_set_->cancel();
  }

private:
  ThreadModel& tm_;
  BackendConfig back_conf_;
  FrontendConfig front_conf_;
  Uptr<Backend> backend_;
  Uptr<Frontend> frontend_;
  Uptr<boost::asio::signal_set> signal_set_;
};


// back-bot-addr
// front-admin-addr
int zbackfront_main(int argc, char* argv[]) {
  KeyedCmdLine<char> cl(argc, argv);
  int ret{ 0 };

  LogConfigFromDict log_conf;
  ThreadModelConfigFromDict tm_conf;
  TcpEndpoint
    back_bot_addr("127.0.0.1", 20000),
    front_admin_addr("127.0.0.1", 30000);

  tm_conf.num_threads_default = std::thread::hardware_concurrency() * 2;

  try {
    log_conf = LogConfigFromDict(LogConfig(), cl.GetNamedArgs(), ConsumeAction::kConsume);
    OverrideFromDict<string, string, TcpEndpoint>(cl.GetNamedArgs(), "back-bot-addr", back_bot_addr, ConsumeAction::kConsume);
    OverrideFromDict<string, string, TcpEndpoint>(cl.GetNamedArgs(), "front-admin-addr", back_bot_addr, ConsumeAction::kConsume);
  }
  catch (LogConfigException& e) {
    cout << "ConfigSet exception: " << e.what() << "\n";
    return -1;
  }
  InitLogWithConfig(log_conf);

  syslog(_INFO) << "Using " << tm_conf.num_threads_default << " threads\n";
  syslog(_INFO) << "Backend CC addr:    " << back_bot_addr.ToString() << "\n";
  syslog(_INFO) << "Frontend admin addr:    " << front_admin_addr.ToString() << "\n";

  BackendConfig back_conf;
  FrontendConfig front_conf;
  back_conf.cc_server_locaddr = back_bot_addr;
  front_conf.frontend_server_locaddr = front_admin_addr;

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
        auto obj(make_unique<ZbackfrontObject>(_tm_, back_conf, front_conf));
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
  return zbackfront_main(argc, argv);
}

