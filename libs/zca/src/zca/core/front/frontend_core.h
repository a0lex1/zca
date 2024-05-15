#pragma once

#include "zca/core/front/frontend_api.h"
#include "zca/frontend_config.h"
#include "zca/core_facade.h"

#include "zca/pki/pki_factory.h"

#include "zca/engine/module_engine.h"

#include "netshell/netshell_factory.h"

#include "co/async/server.h"
#include "co/async/loop_object.h"
#include "co/async/service.h"
#include "co/xlog/xlog.h"

DECLARE_XLOGGER_SINK("frontend", gFrontendLogSink);

namespace core {
namespace front {

struct FrontendBuildTypes
{
  using StreamAcceptor = co::async::StreamAcceptor;
  using StreamFactory = co::async::StreamFactory;
  using StreamConnector = co::async::StreamConnector;

  struct SeparatableObjects {
    // We're serial netshell client
    Uptr<StreamAcceptor> admin_server_acceptor;
    Shptr<StreamFactory> admin_stream_factory;    
    Uptr<netshell::NsServerFactory> netshell_fac;

    // We're parallel backshell client
    Shptr<StreamConnector> bk_ns_connector;
    Uptr<StreamFactory> bk_stream_factory;
    Uptr<netshell::NsClientFactory> bk_ns_factory;
    
    io_context* ioc_admin_sess_strands;
    io_context* ioc_module_strands;

    Shptr<Strand> tss_strand;
    Shptr<Strand> module_engine_strand;
  };

  struct Objects {
    Uptr<pki::PkiFactory> pki_fac_;
  };

  struct Params {
    FrontendConfig user_config;
  };

  struct SharedData {
  };
};

class FrontendCore :
  public Buildable<FrontendBuildTypes>,
  public co::async::LoopObjectNoreset,
  private co::async::Stopable
{
public:
  virtual ~FrontendCore() = default;

  using RefTracker = co::RefTracker;
  using RefTrackerContext = co::RefTrackerContext;
  using RefTrackerContextHandle = co::RefTrackerContextHandle;
  using Endpoint = co::net::Endpoint;
  using Session = co::async::Session;
  using Stream = co::async::Stream;
  using ThreadsafeStopableImpl = co::async::ThreadsafeStopableImpl;
  
  using BasicJobManager = engine::BasicJobManager;

  FrontendCore();

  // AddModule cannot be called after PrepareToStart() !
  void AddModule(Uptr<engine::Module> mod);

  void GetLocalAddressToConnect(Endpoint&, Errcode&); // admin

  // [LoopObject impl]
  void PrepareToStart(Errcode&) override;
  void Start(RefTracker rt) override;
  void StopThreadsafe() override;
  void CleanupAbortedStop() override;

  Endpoint GetLocalAddressToConnect();


private:
  // [Stopable impl]
  void StopUnsafe() override;

  // [Buildable<...> impl]
  void CompleteBuild() override;

  using ServerWithSessList = co::async::ServerWithSessList;

  Shptr<Session> AdminSessionFactoryFunc(Uptr<Stream>, Shptr<Strand>, RefTrackerContextHandle);

private:
  Uptr<ThreadsafeStopableImpl> tss_impl_;
  Uptr<engine::ModuleEngine> module_engine_;
  Uptr<ServerWithSessList> admin_server_;
  bool prepared_{ false };
  Uptr<FrontendGlobalApi> global_api_;
  
  //std::atomic<bool> _dbg_modules_stopped_{ false };
  std::atomic<bool> _dbg_servers_stopped_{ false };
  std::atomic<bool> _dbg_adminserver_stopped_{ false };
  
};

}}