#pragma once

#include "zca/core/cc_filter_chain.h"
#include "zca/core/back/backend_api.h"

#include "zca/backend_config.h"
#include "zca/core_facade.h"

#include "zca/engine/module_engine.h"

#include "cc/cc_server.h"

#include "netshell/netshell_factory.h"

#include "co/async/loop_object.h"
#include "co/async/server.h"
#include "co/base/filter_chain.h"
#include "co/xlog/define_logger_sink.h"

DECLARE_XLOGGER_SINK("backend", gBackendLogSink);

namespace core {
namespace back {

struct BackendBuildTypes
{
  using StreamAcceptor = co::async::StreamAcceptor;
  using StreamFactory = co::async::StreamFactory;

  struct SeparatableObjects {
    Uptr<StreamAcceptor> admin_server_acceptor;
    Shptr<StreamFactory> admin_stream_factory;
    Uptr<StreamAcceptor> cc_server_acceptor;
    Shptr<StreamFactory> cc_stream_factory;
    Uptr<netshell::NsServerFactory> netshell_fac; // abstracts admin protocol, can be JSON (if you're nerd)
    io_context* ioc_admin_sess_strands;
    io_context* ioc_cc_sess_strands;
    io_context* ioc_module_strands;
    //Shptr<Strand> taskmgr_strand;
    Shptr<Strand> tss_strand;
    Shptr<Strand> module_engine_strand;
  };

  struct Objects {
  };

  struct Params {
    BackendConfig user_config;
  };

  struct SharedData {
  };
};

class BackendCore :
  public Buildable<BackendBuildTypes>,
  public co::async::LoopObjectNoreset,
  private CcServerEventsLink,
  private co::async::Stopable
{
public:
  virtual ~BackendCore() = default;

  using RefTracker = co::RefTracker;
  using RefTrackerContext = co::RefTrackerContext;
  using RefTrackerContextHandle = co::RefTrackerContextHandle;
  using Endpoint = co::net::Endpoint;
  using Session = co::async::Session;
  using Stream = co::async::Stream;
  using TaskManager = co::async::TaskManager;
  using ThreadsafeStopableImpl = co::async::ThreadsafeStopableImpl;

  using BasicJobManager = engine::BasicJobManager;

  BackendCore();

  // AddModule cannot be called after PrepareToStart() !
  void AddModule(Uptr<engine::Module> mod);

  void GetLocalAddressToConnect(Endpoint&, Errcode&); // admin
  void GetLocalCcAddressToConnect(Endpoint&, Errcode&);

  // [LoopObject impl]
  void PrepareToStart(Errcode&) override;
  void Start(RefTracker rt) override;
  void StopThreadsafe() override;
  void CleanupAbortedStop() override;
  // connect your stupid events here
  CcServerEventsFilterChainHead& GetCcServerEventsFilterChain();

  Endpoint GetLocalAddressToConnect();
  Endpoint GetLocalCcAddressToConnect();


private:
  // [Stopable impl]
  void StopUnsafe() override;

  // [Buildable<...> impl]
  void CompleteBuild() override;

  // [CcServerEventsLink impl]
  void OnBotHandshakeComplete(Shptr<cc::ICcBot>) override;

  using ServerWithSessList = co::async::ServerWithSessList;

  Shptr<Session> AdminSessionFactoryFunc(Uptr<Stream>, Shptr<Strand>, RefTrackerContextHandle);
  void AllocBotModuleDataSlots(Shptr<cc::ICcBot>);
  void CallModulesOnBotHandshake(Shptr<cc::ICcBot>);
  void CallModulesRegisterBotProperties();

private:
  Uptr<ThreadsafeStopableImpl> tss_impl_;
  //RefTrackerContext            rtctx_; // now Start's |rt| is used (it's for taskmgr)
  Uptr<cc::CcServer>           cc_server_;
  Uptr<engine::ModuleEngine>   module_engine_;
  Uptr<ServerWithSessList>     admin_server_;
  bool                         prepared_{ false };
  CcServerEventsFilterChainHead event_chain_head_;
  Uptr<BackendGlobalApi>       global_api_;
  BotPropertyGroupSet          bot_propg_set_;
  BotPropertyGroupRegistrator  bot_propg_set_reg_;


  std::atomic<bool> _dbg_modules_stopped_{ false };
  std::atomic<bool> _dbg_servers_stopped_{ false };
  std::atomic<bool> _dbg_ccserver_stopped_{ false };
  std::atomic<bool> _dbg_adminserver_stopped_{ false };
};

}}
