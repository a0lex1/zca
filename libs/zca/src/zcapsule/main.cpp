#include "zca/modules/basecmd/back/basecmd_backend_module.h"
#include "zca/modules/basecmd/front/basecmd_frontend_module.h"
#include "zca/modules/basecmd/ag/basecmd_agent_module.h"

#include "zca/configs/backend_config_from_dict.h"
#include "zca/configs/agent_config_from_dict.h"

#include "zca/dbg/dbgconsole/zca_debug_console_module.h"

#include "zca/triplet.h"

#include "co/async/capsule/capsule_builder.h"
#include "co/async/configs/thread_model_config_from_dict.h"
#include "co/async/tcp_service.h"

#include "co/dbg/dbgconsole/debug_console.h"
#include "co/dbg/dbgconsole/co_debug_console_module.h"

#include "co/base/cmdline/keyed_cmdline.h"

#include "co/xlog/configs.h"
#include "co/xlog/xlog.h"

#include <boost/asio/signal_set.hpp>
#include <iostream>

using namespace std;
using namespace boost::asio;
using namespace co;
using namespace co::xlog::configs;
using namespace co::cmdline;
using namespace co::async;
using namespace co::async::capsule;
using namespace co::async::configs;
using namespace co::net;

class ZcapsuleExeObject :
  public LoopObject,
  private core::CcServerEventsLink,
  private core::CcClientEventsLink
{
public:
  virtual ~ZcapsuleExeObject() = default;
  
  ZcapsuleExeObject(ThreadModel& tm)
    :
    tm_(tm), triplet_(tm, nullptr)
  {
  }

  void SetInjectErrorInPrepare() {
    inject_error_ = true;
  }

  Triplet& GetTriplet() {
    return triplet_;
  }

  // [LoopObject::LoopObjectBase impl]
  void PrepareToStart(Errcode& ) override {
    if (inject_error_) {
      BOOST_THROW_EXCEPTION(std::runtime_error("INJECTED ERROR"));
    }

    debug_console_service_ = make_shared<co::async::TcpService>(tm_.DefIOC());

    debug_console_ = make_unique<co::dbg::dbgconsole::DebugConsole>();
    debug_console_->Configure(co::net::TcpEndpoint("127.0.0.1:5000"), debug_console_service_);
    debug_console_->AddModule(make_unique<co::dbg::dbgconsole::CoDebugConsoleModule>());
    debug_console_->AddModule(make_unique<zca::dbg::dbgconsole::ZcaDebugConsoleModule>());
    debug_console_->PrepareToStartNofail();

    triplet_.SetAddresses(TcpEndpoint("0.0.0.0", 10000),
      TcpEndpoint("0.0.0.0", 20000),
      TcpEndpoint("127.0.0.1", 30000));

    triplet_.EnableBackend(this);
    triplet_.EnableAgent(this);
    triplet_.EnableFrontend();


    triplet_.AddModuleTriplet(
      make_unique<modules::basecmd::back::BasecmdBackendModule>(),
      make_unique<modules::basecmd::front::BasecmdFrontendModule>(),
      make_unique<modules::basecmd::ag::BasecmdAgentModule>());

    triplet_.GetAgentConfig().bot_id = cc::BotId::FromUint(0);
    triplet_.PrepareToStartNofail();

    // after PrepareToStart() acceptors are already bound
    // print those to stdout so stupid user can see it and connect
    // with his damn netcat
    auto back_admin_addr(triplet_.GetBackend().GetLocalAddressToConnect());
    auto back_cc_addr(triplet_.GetBackend().GetLocalCcAddressToConnect());
    auto front_admin_addr(triplet_.GetFrontend().GetLocalAddressToConnect());
    auto dbgconsole_addr(debug_console_->GetLocalAddressToConnect());
    cout
      << "###### ###### ****** ###### ##\n"
      << "Backend admin at " << back_admin_addr.ToString() << "\n"
      << "###### ****** ###### ****** ######\n"
      << "Backend CC at " << back_cc_addr.ToString() << "\n"
      << "###### ###### ****** ###### ##you#\n"
      << "Dbgconsole at " << dbgconsole_addr.ToString() << "\n"
      << "###### ######        ###### ######\n"
      <<  "Frontend admin at " << front_admin_addr.ToString() << "\n"
      "   ###### ######        ###### ######\n"
      ;

    signal_set_ = make_unique<signal_set>(triplet_
      .GetThreadModel().DefIOC(),
      SIGINT, SIGTERM /*, SIGQUIT*/);
  }

  void CleanupAbortedStop() override {
    triplet_.CleanupAbortedStop();
    debug_console_->CleanupAbortedStop();
  }
  bool IsResetSupported() const override {
    return triplet_.IsResetSupported() && debug_console_->IsResetSupported();
  }
  void ResetToNextRun() override {
    DCHECK(IsResetSupported());
    triplet_.ResetToNextRun();
    debug_console_->ResetToNextRun();
  }
  // [LoopObject::LoopObject impl]
  void Start(RefTracker rt) override {
    signal_set_->async_wait([&](Errcode err, int sig) {
      if (err) {
        cout << "ERROR: Mo more waiting for signals (" << err << ")\n";
        return;
      }
      cout << "WARNING: Signal " << sig << " caught. Stopping...\n";
      
      StopThreadsafe();
                      });


    triplet_.Start(rt);
    debug_console_->Start(rt);

  }
  void StopThreadsafe() override {
    triplet_.StopThreadsafe();
    debug_console_->StopThreadsafe();
  }
private:
  // [CcServerEvents impl]
  void OnBotHandshakeComplete(Shptr<cc::ICcBot> bot) override {
    cout << "bot hshake complete " << bot->GetReadonlyData().GetHandshakeData()->GetBotId().ToStringRepr() << "\n";
  }
  void OnBotRemovedFromList(Shptr<cc::ICcBot> bot) override {
    if (bot->GetReadonlyData().GetHandshakeData() != nullptr) {
      cout << "bot removed " << bot->GetReadonlyData().GetHandshakeData()->GetBotId().ToStringRepr() << "\n";
    }
    else {
      cout << "bot removed (without botid)\n";
    }
  }
  // [CcClientEvents impl]
  void OnClientHandshakeWritten() override {
    cout << "client handshake written \n";
  }
  void OnClientHandshakeReplyReceived() override {
    cout << "client hshake REPLY received\n";
  }

private:
  ThreadModel& tm_;
  Triplet triplet_;
  Shptr<co::async::Service> debug_console_service_;
  Uptr<co::dbg::dbgconsole::DebugConsole> debug_console_;
  bool inject_error_{ false };
  Uptr<boost::asio::signal_set> signal_set_;
};

// --------------------------------------------------------------------------------------------------------------------------------

//#define USE_LOOP_OBJECT_SET

int zcapsule_main(int argc, char* argv[]) {
  KeyedCmdLine<char> cl(argc, argv);
  int ret{ 0 };
  try {
    InitLogWithConfig(LogConfigFromDict(LogConfig(), cl.GetNamedArgs(), ConsumeAction::kConsume));
    ThreadModelConfigFromDict tm_conf(ThreadModelConfig(), cl.GetNamedArgs(), ConsumeAction::kConsume);

    bool inject_error = false;
    OverrideFromDict<string, string, bool>(cl.GetNamedArgs(),
                                           "inject-error", inject_error, ConsumeAction::kConsume);

    TcpEndpoint back_admin_addr("127.0.0.1", 10000),
      back_bot_addr("127.0.0.1", 20000),
      front_admin_addr("127.0.0.1", 30000);
    OverrideFromDict<string, string, TcpEndpoint>(cl.GetNamedArgs(), "back-admin-addr", back_admin_addr, ConsumeAction::kConsume);
    OverrideFromDict<string, string, TcpEndpoint>(cl.GetNamedArgs(), "back-bot-addr", back_bot_addr, ConsumeAction::kConsume);
    OverrideFromDict<string, string, TcpEndpoint>(cl.GetNamedArgs(), "front-admin-addr", back_bot_addr, ConsumeAction::kConsume);
    cout << "Backend admin addr: " << back_admin_addr.ToString() << "\n";
    cout << "Backend CC addr:    " << back_bot_addr.ToString() << "\n";
    cout << "Frontend admin addr:    " << front_admin_addr.ToString() << "\n";

    cout
      << "==============================================================\n"
      << "Ctrl + C to stop and restart, Ctrl + C again to stop + exit.\n"
      << "==============================================================\n";
    static const size_t kMaxNormalExitIters = 2;
    static const size_t kMaxTotalIters = LoopOptions::kUnlimitedIterations; // default

    // build a capsule
    CapsuleBuilder builder;
    builder
      .SetLoopOptionsCreator([]() {
                               return make_unique<LoopOptions>(kMaxTotalIters,
                                                               kMaxNormalExitIters);
                             })
      .SetIoInitiatorCreator([&](ThreadModel& _tm_) {
                               auto exeobj(make_unique<ZcapsuleExeObject>(_tm_));
                               if (inject_error) {
                                 exeobj->SetInjectErrorInPrepare();
                               }
                               exeobj->GetTriplet().SetAddresses(
                                 back_admin_addr,
                                 back_bot_addr,
                                 front_admin_addr);
                               return exeobj;
                             })
      .SetThreadModelCreator([&]() {
                               return make_unique<ThreadModel>(tm_conf);
                             });

    Capsule capsule(builder.GetObjectFactory());

    // link it and run
    capsule.Initialize();
    capsule.Run();
  }
  catch (LogConfigException& e) {
    cout << "ConfigSet exception: " << e.what() << "\n";
    ret = -1;
  }
  catch (std::exception& e) {
    cout << "Exception: std::exception " << e.what() << "\n";
    ret = -2;
  }

  if (!cl.GetNamedArgs().empty()) {
    cout << "P.S.: Unknown args left:\n";
    KeyedCmdLine<char>::PrintUnknownArgsLeft(cout, cl.GetNamedArgs(), "\t");
    return -3;
  }

  return ret;
}

int main(int argc, char* argv[]) {
  return zcapsule_main(argc, argv);
}


