#include "zca/triplet.h"

#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;
using co::net::Endpoint;
using co::net::TcpEndpoint;

DEFINE_XLOGGER_SINK("triplet", gZcaTripletSink);
#define XLOG_CURRENT_SINK gZcaTripletSink

Triplet::Triplet(ThreadModel &tm, TripletEvents *events)
  :
    tss_impl_(*this, make_shared<Strand>(tm.DefIOC())),
    tm_(tm), events_(events)
{
  SetAddresses(
    TcpEndpoint("127.0.0.1", 0),
    TcpEndpoint("127.0.0.1", 0),
    TcpEndpoint("127.0.0.1", 0));
}

void Triplet::SetAddresses(Endpoint back_admin_addr, Endpoint back_bot_addr,
  Endpoint front_admin_addr)
{
  backend_conf_.admin_server_locaddr = back_admin_addr;
  backend_conf_.cc_server_locaddr = back_bot_addr;
  frontend_conf_.frontend_server_locaddr = front_admin_addr;
}

void Triplet::EnableBackend(core::CcServerEventsLink* server_events)
{
  DCHECK(!IsBackendEnabled());
  enable_backend_ = true;
  user_server_events_link_ = server_events;
}

void Triplet::EnableAgent(core::CcClientEventsLink* client_events)
{
  DCHECK(!IsAgentEnabled());
  enable_agent_ = true;
  user_client_events_link_ = client_events;
}

void Triplet::EnableFrontend()
{
  DCHECK(!IsFrontendEnabled());
  enable_frontend_ = true;
}

void Triplet::AddModuleTriplet(Uptr<Module> backend_part,
                               Uptr<Module> frontend_part,
                               Uptr<Module> agent_part)
{
  module_tuple_list_.emplace_back(make_tuple(
    std::move(backend_part),
    std::move(frontend_part),
    std::move(agent_part)
  ));
}

void Triplet::PrepareToStart(Errcode& err)
{
  DCHECK(!prepared_to_start_);
  DCHECK(!backend_);
  DCHECK(!agent_);

  // ---------------------------------------------------------------------------------------------------
  DCHECK(enable_backend_); // Backend must be enabled

  // Create backend
  backend_ = make_unique<Backend>(tm_, backend_conf_, backend_sep_conf_);

  // Connect backend user events if set
  if (user_server_events_link_ != nullptr) {
    backend_->GetCcServerEventsFilterChain().AttachTop(
      user_server_events_link_);
  }
  // Add backend modules
  for (auto& module_tuple : module_tuple_list_) {
    Uptr<Module>& modp = std::get<0>(module_tuple);
    if (modp != nullptr) {
      backend_->AddModule(std::move(modp));
    }
  }

  // Need to PrepareToStart the backend first, because we need a port number
  Log(_DBG) << "Doing backend_->PrepareToStart() to bind\n";

  err = NoError();
  backend_->PrepareToStart(err);
  if (err) {
    Log(_ERR) << "backend->PrepareToStart() returned err " << err << "\n";
    // **********************************************************************
    // We CAN interrupt PrepareToStart() because we are LoopObjectNoreset
    // nobody will reset us, they will just recreate
    // so bad states are ok
    // **********************************************************************
    return;
  }

  // ---------------------------------------------------------------------------------------------------

  if (enable_agent_) {
    // Create agent, we have server's bound address
    Endpoint back_cc_addr(backend_->GetLocalCcAddressToConnect());
    agent_conf_.back_remaddr = back_cc_addr;

    // If configured with preset bot id, use it
    agent_ = make_unique<Agent>(tm_, agent_conf_, agent_sep_conf_);
    //SET_DEBUG_TAG(*agent_, "%s", agent_debug_tag_.c_str());

    if (user_client_events_link_ != nullptr) {
      agent_->GetCcClientEventsFilterChain().AttachTop(user_client_events_link_);
    }
    // Add agent modules
    for (auto& module_tuple : module_tuple_list_) {
      Uptr<Module>& modp = std::get<2>(module_tuple);
      if (modp != nullptr) {
        agent_->AddModule(std::move(modp));
      }
    }

    agent_->PrepareToStart(err);
    if (err) {
      Log(_ERR) << "agent->PrepareToStart() returned err " << err << "\n";
      return;
    }
  }

  // ---------------------------------------------------------------------------------------------------
  
  if (enable_frontend_) {
    DCHECK(enable_backend_); // Frontend depends on backend
    frontend_conf_.bk_addr = backend_->GetLocalAddressToConnect();
    frontend_ = make_unique<Frontend>(tm_, frontend_conf_, frontend_sep_conf_);
    // Add frontend modules
    for (auto& module_tuple : module_tuple_list_) {
      Uptr<Module>& modp = std::get<1>(module_tuple);
      if (modp != nullptr) {
        frontend_->AddModule(std::move(modp));
      }
    }
    frontend_->PrepareToStart(err);
    if (err) {
      Log(_ERR) << "frontend->PrepareToStart() returned err " << err << "\n";
      return;
    }
  }

  prepared_to_start_ = true;
}

void Triplet::CleanupAbortedStop() {
  DCHECK(prepared_to_start_);
  if (IsBackendEnabled()) {
    GetBackend().CleanupAbortedStop();
  }
  if (IsAgentEnabled()) {
    GetAgent().CleanupAbortedStop();
  }
  if (IsFrontendEnabled()) {
    GetFrontend().CleanupAbortedStop();
  }
}

void Triplet::Start(RefTracker rt) {
  DCHECK(prepared_to_start_);

  tss_impl_.BeforeStartWithIoEnded(rt, rt);

  RefTracker rt_all(CUR_LOC(),
                    [&] () {
      Log(_DBG) << "Triplet #ioended\n";
    },
    rt);

  if (backend_) {
    RefTracker rt_back(RefTracker(CUR_LOC(), [&]() {
      Log(_DBG) << "Triplet's backend #ioended\n";
      if (events_) {
        // Call user's events
        events_->OnBackendIoEnded();
      }
      }, rt_all));

    backend_->Start(rt_back);
  }

  if (agent_) {
    RefTracker rt_agent(RefTracker(CUR_LOC(), [&]() {

      if (agent_->GetConnectError()) {
        Log(_ERR) << "Triplet's agent " << SPTR(agent_.get()) << " -=Can't Connect=- #ioended, "
                  << "GetConnectError() = " << agent_->GetConnectError() << "\n";
        if (events_) {
          // Was in ZcaTestObject::Start::lambdaXXX
          events_->OnAgentConnectionFailed(agent_->GetConnectError());
        }

        // this means need stop, what's the point of running triplet without agent
        StopThreadsafe();
      }
      Log(_DBG) << "Triplet's agent " << SPTR(agent_.get()) << " #ioended\n";
      // Always call user's events, no matter it was connect error or not
      if (events_) {
        events_->OnAgentIoEnded();
      }
                        }, rt_all));
    agent_->Start(rt_agent);

  } // if (agent_)

  if (frontend_) {
    RefTracker rt_front(RefTracker(CUR_LOC(), [&]() {
      Log(_DBG) << "Triplet's frontend #ioended\n";
      if (events_) {
        // Call user's events
        events_->OnFrontendIoEnded();
      }
      }, rt_all));

    frontend_->Start(rt_front);
  }

  if (events_) {
    // Call user's events
    events_->OnTripletStarted();
  }
}

void Triplet::StopThreadsafe() {
  tss_impl_.StopThreadsafe();
}

void Triplet::StopUnsafe()  {
  // Called from tss_impl_
  if (backend_) {
    Log(_DBG) << "Triplet is stopping backend...\n";
    backend_->StopThreadsafe();
  }
  else {
    Log(_DBG) << "Triplet's backend not started, no need to stop\n";
  }

  if (agent_) {
    Log(_DBG) << "Triplet is stopping agent...\n";
    agent_->StopThreadsafe();
  }
  else {
    Log(_DBG) << "Triplet's agent not started, no need to stop\n";
  }

  if (frontend_) {
    Log(_DBG) << "Triplet is stopping frontend...\n";
    frontend_->StopThreadsafe();
  }
}











