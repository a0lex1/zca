#include "zca/backend.h"
#include "zca/agent.h"
#include "zca/frontend.h"

#include "zca/core/cc_filter_chain.h"

#include "cc/cc_client_events_from_func.h"
#include "cc/cc_server_events_from_func.h"

#include "co/async/threadsafe_stopable_impl.h"

DECLARE_XLOGGER_SINK("triplet", gZcaTripletSink);

class TripletBase {
public:
  virtual ~TripletBase() = default;
  
  virtual void EnableBackend(core::CcServerEventsLink* server_events) = 0;
  virtual void EnableAgent(core::CcClientEventsLink* client_events) = 0;
  virtual void EnableFrontend() = 0;

  virtual void AddModuleTriplet(
    Uptr<engine::Module>,
    Uptr<engine::Module>,
    Uptr<engine::Module>) = 0;
};

class TripletEvents {
public:
  virtual ~TripletEvents() = default;

  virtual void OnTripletStarted() = 0;
  virtual void OnAgentConnectionFailed(Errcode) = 0;
  virtual void OnBackendIoEnded() = 0;
  virtual void OnAgentIoEnded() = 0; // Called even if OnAgentConnectionFailed() was called
  virtual void OnFrontendIoEnded() = 0;
};

class Triplet
  :
  public TripletBase,
  public co::async::LoopObjectNoreset,
  private co::async::Stopable                          // for tss_impl_
{
public:
  virtual ~Triplet() = default;
  
  using Endpoint = co::net::Endpoint;
  using TcpEndpoint = co::net::TcpEndpoint;
  using ThreadModel = co::async::ThreadModel;
  using RefTracker = co::RefTracker;
  using Module = engine::Module;
  using ThreadsafeStopableImpl = co::async::ThreadsafeStopableImpl;

  Triplet(ThreadModel& tm, TripletEvents* events);

  // Endpoint(s) in configs are managed by SetAddresses(). These fields shouldn't be touched
  void SetAddresses(Endpoint back_admin_addr, Endpoint back_bot_addr, Endpoint front_admin_addr);
  
  BackendConfig& GetBackendConfig() { return backend_conf_; }
  FrontendConfig& GetFrontendConfig() { return frontend_conf_; }
  AgentConfig& GetAgentConfig() { return agent_conf_; }

  BackendSeparationConfig& GetBackendSeparationConfig() { return backend_sep_conf_; }
  //FrontendSeparationConfig& GetFrontendSeparationConfig() { return frontend_conf_; }
  //AgentSeparationConfig& GetAgentSeparationConfig() { return agent_sep_conf_; }

  // only after PrepareToStart()
  Backend& GetBackend() { return *backend_.get(); }
  Agent& GetAgent() { return *agent_.get(); }
  Frontend& GetFrontend() { return *frontend_.get(); }

  bool IsBackendEnabled() const { return enable_backend_; }
  bool IsAgentEnabled() const { return enable_agent_; }
  bool IsFrontendEnabled() const { return enable_frontend_; }
  ThreadModel& GetThreadModel() { return tm_; }

  // [TripletBase impl]
  void EnableBackend(core::CcServerEventsLink* server_events = nullptr) override;
  void EnableAgent(core::CcClientEventsLink* client_events = nullptr) override;
  void EnableFrontend();
  void AddModuleTriplet(Uptr<Module> backend_module,
                        Uptr<Module> frontend_module,
                        Uptr<Module> agent_module) override;
  // [LoopObjectNoreset::LoopObject::LoopObjectBase impl]
  void PrepareToStart(Errcode&) override;
  void CleanupAbortedStop() override;
  // [LoopObjectNoreset::LoopObject impl]
  void Start(RefTracker rt) override;
  void StopThreadsafe() override;
  
  // tuple <BackendModule, FrontendModule, AgentModule>
  using VectorOfTuples = std::vector<std::tuple<Uptr<Module>, Uptr<Module>, Uptr<Module> > >;

private:
  // [Stopable impl]
  void StopUnsafe() override;

private:
  ThreadsafeStopableImpl tss_impl_;
  ThreadModel& tm_;
  TripletEvents* events_;

  bool enable_backend_{ false };
  bool enable_agent_{ false };
  bool enable_frontend_{ false };
  
  BackendConfig backend_conf_;
  BackendSeparationConfig backend_sep_conf_;
  AgentConfig agent_conf_;
  AgentSeparationConfig agent_sep_conf_;
  FrontendConfig frontend_conf_;
  FrontendSeparationConfig frontend_sep_conf_;
  
  core::CcServerEventsLink* user_server_events_link_{ nullptr };
  core::CcClientEventsLink* user_client_events_link_{ nullptr };
  bool prepared_to_start_{ false };

  Uptr<Backend> backend_;
  Uptr<Agent> agent_;
  Uptr<Frontend> frontend_;

  VectorOfTuples module_tuple_list_;
};

