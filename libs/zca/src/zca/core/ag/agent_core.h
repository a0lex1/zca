#pragma once

#include "zca/core/cc_filter_chain.h"
#include "zca/core/ag/agent_api.h"

#include "zca/engine/module_engine.h"

#include "zca/pki/pki_factory.h"

#include "zca/agent_config.h"
#include "zca/core_facade.h"

#include "cc/cc_client.h"

#include "co/async/loop_object.h"
#include "co/async/client.h"
#include "co/base/debug_tag_owner.h"
#include "co/base/filter_chain.h"
#include "co/xlog/define_logger_sink.h"


namespace core {
namespace ag {

DECLARE_XLOGGER_SINK("agent", gAgentLogSink);

struct AgentBuildTypes
{
  struct SeparatableObjects {
    io_context* ioc_eng_sess;
    Uptr<co::async::Stream> client_stream;
    Shptr<Strand> cc_session_strand;
    Shptr<Strand> taskmgr_strand;
    Shptr<Strand> jobmgr_strand;
    Shptr<Strand> tss_strand;
    Shptr<Strand> engsess_strand;
  };

  struct Objects {
    Uptr<pki::PkiFactory> pki_fac_;
  };

  struct Params {
    AgentConfig user_config; // original config
  };

  struct SharedData {
  };
};

class AgentCore
  :
  public Buildable<AgentBuildTypes>,
  public co::async::LoopObjectNoreset,
  private cc::CcClientCommandDispatcher,
  protected co::async::Stopable,
  public co::DebugTagOwner
{
public:
  using RefTracker = co::RefTracker;
  using RefTrackerContext = co::RefTrackerContext;
  using Endpoint = co::net::Endpoint;
  using TaskManager = co::async::TaskManager;
  using ThreadsafeStopableImpl = co::async::ThreadsafeStopableImpl;

  virtual ~AgentCore();

  AgentCore();

  void AddModule(Uptr<engine::Module> mod);

  // You can use any value in config's backend_remote_addr field
  // And then, right before Start(), replace it by a call to following func
  void SetBackendCcRemoteAddr(const Endpoint&);

  // [LoopObject impl]
  void PrepareToStart(Errcode&) override;
  void Start(RefTracker rt) override;
  void StopThreadsafe() override;
  void CleanupAbortedStop() override;

  Errcode GetConnectError() const { return cc_client_->GetConnectError(); }

  // Only after ioended
  const std::vector<cc::CcError>& GetCcClientLastErrorStack() const {
    return cc_client_->GetLastErrorStack(); 
  }

  // connect your events here
  CcClientEventsFilterChainHead& GetCcClientEventsFilterChain();

  const cc::CcClientBotOptions& GetClientBotOptions() const;

private:
  // [Stopable impl]
  void StopUnsafe() override;

  // [Buildable<...> impl]
  void CompleteBuild() override;

  // [CcClientCommandDispatcher impl] - called by CcClient
  void DispatchCommand(
    Uptr<std::string> cmd_opaque_data,
    std::string& cmd_result_opaque_data,
    EmptyHandler handler) override;

  void StartObjects(RefTracker _rt);
  void HandleExecuteCommand();

  bool IsInsideTssStrand() { return GetTssStrand()->running_in_this_thread(); }
  Shptr<Strand> GetTssStrand() { return tss_impl_->GetStrandShptr(); }

  cc::BotId GetBotId() const {
    return cc_client_->GetBotOptions().GetBotId();
  }

  std::string SerializedResultWithEmptySalt(const netshell::NsCmdResult&);

private:
  struct {
    bool prepared_to_start_ : 1;
    bool started_ : 1;
  } bools_ { false, false };
  Uptr<ThreadsafeStopableImpl> tss_impl_;
  Uptr<cc::CcClient> cc_client_;
  Uptr<engine::ModuleEngine> module_engine_;
  CcClientEventsFilterChainHead event_chain_head_;
  Uptr<engine::EngineSession> engine_session_;
  Uptr<netshell::NsCmdResult> cmd_result_;
  std::string* user_cmd_result_opaque_data_;
  EmptyHandler user_handler_;
  Uptr<AgentGlobalApi> global_api_;
  Uptr<pki::SaltGenerator> salt_generator_;
  Uptr<pki::SignatureVerifier> sig_verifier;
  std::string current_salt_;

  std::atomic<bool> _dbg_modules_stopped_{ false };
  std::atomic<bool> _dbg_engsess_stopped_{ false };
  std::atomic<bool> _dbg_client_stopped_{ false };
};

}}






