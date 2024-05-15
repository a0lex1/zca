#include "zca/core/back/backend_core.h"
#include "zca/core/back/backend_builder.h"
#include "zca/core/back/backend_bot_user_data.h"
#include "zca/core/back/backend_admin_session.h"
#include "zca/zca_common_config.h"

#include "zca/netshell_status_descriptor_table.h"

//#include "co/async/test_kit/test_session.h"
#include "co/async/create_for_endpoint.h"
#include "co/base/strings.h"
#include "co/xlog/xlog.h"

using namespace co;
using namespace co::async;
using namespace std;
using namespace cc;
using co::net::Endpoint;

namespace core {
namespace back {

DEFINE_XLOGGER_SINK("backend", gBackendLogSink);
#define XLOG_CURRENT_SINK gBackendLogSink

#define llog() Log(_DBG) << "Bk " << SPTR(this) << " `" << "" << "` "

Uptr<BackendBuilder::TSepObjects> BackendBuilder::BuildSeparatableObjects(
  ThreadModel & tm,
  const BackendConfig& conf,
  const BackendSeparationConfig& sep)
{
  auto sepobjs = make_unique<BackendBuildTypes::SeparatableObjects>();

  sepobjs->admin_server_acceptor = CreateStreamAcceptorForEndpoint(
    tm.AcquireIoContextForThreadGroup(sep.admin_acceptor_threadgroup),
    conf.admin_server_locaddr);

  SET_DEBUG_TAG(sepobjs->admin_server_acceptor->DebugTag(), "back_admin_srv_acpt");

  sepobjs->admin_stream_factory = CreateStreamFactoryForEndpoint(
    tm.AcquireIoContextForThreadGroup(sep.admin_sessions_threadgroup),
    conf.admin_server_locaddr);

  sepobjs->cc_server_acceptor = CreateStreamAcceptorForEndpoint(
    tm.AcquireIoContextForThreadGroup(sep.cc_acceptor_threadgroup),
    conf.cc_server_locaddr);

  SET_DEBUG_TAG(sepobjs->cc_server_acceptor->DebugTag(), "cc_srv_acpt");

  sepobjs->cc_stream_factory = CreateStreamFactoryForEndpoint(
    tm.AcquireIoContextForThreadGroup(sep.cc_sessions_threadgroup),
    conf.cc_server_locaddr);

  sepobjs->netshell_fac = make_unique<netshell::NsServerFactoryText>();

  sepobjs->ioc_admin_sess_strands = &tm.AcquireIoContextForThreadGroup(
    sep.admin_sessions_threadgroup);

  sepobjs->ioc_cc_sess_strands = &tm.AcquireIoContextForThreadGroup(
    sep.cc_sessions_threadgroup);

  sepobjs->ioc_module_strands = &tm.AcquireIoContextForThreadGroup(
    sep.modules_threadgroup);

  //sepobjs->taskmgr_strand = make_shared<Strand>(
  //  tm.AcquireIoContextForThreadGroup(
  //  sep.taskmgr_threadgroup));

  sepobjs->tss_strand = make_shared<Strand>(
    tm.AcquireIoContextForThreadGroup(
    sep.tss_threadgroup));

  return sepobjs;
}

Uptr<BackendBuilder::TObjects> BackendBuilder::BuildObjects(ThreadModel& tm,
                                                            const BackendConfig&) {
  return make_unique<TObjects>();
}

Uptr<BackendBuilder::TParams> BackendBuilder::BuildParams(const BackendConfig& conf) {
  auto par = make_unique<TParams>();
  par->user_config = conf;
  return par;
}

// BackendCore

BackendCore::BackendCore()
  :
  bot_propg_set_reg_(bot_propg_set_)
{
}

// [Buildable::CompleteBuild]
void BackendCore::CompleteBuild() {

  // CREATE ADMIN SERVER
  admin_server_ = make_unique<ServerWithSessList>(
    Params().user_config.admin_server_locaddr,
    ServerObjects(
      SeparatableObjects().admin_stream_factory,
      std::move(SeparatableObjects().admin_server_acceptor),
      *SeparatableObjects().ioc_cc_sess_strands));

  // CREATE CC SERVER
  cc_server_ = make_unique<CcServer>(
    Params().user_config.cc_server_locaddr,
    ServerObjects(
      SeparatableObjects().cc_stream_factory,
      std::move(SeparatableObjects().cc_server_acceptor),
      *SeparatableObjects().ioc_cc_sess_strands),
    &event_chain_head_,
    Params().user_config.max_chunk_body_size);

  if (Params().user_config.ping_interval.is_zero()) {
    cc_server_->DisablePinging();
  }
  else {
    cc_server_->SetPingInterval(Params().user_config.ping_interval);
  }
  cc_server_->SetTrafficEncryptionKeys("unique", 6, "options", 6);

  // Set debug tags
  SET_DEBUG_TAG(admin_server_->_DbgTag(), string_printf("bk_admsrv_%s", SPTR(admin_server_.get()).c_str()).c_str());
  //SET_DEBUG_TAG(cc_server_->_DbgTag(), string_printf("bk_ccsrv_%s", SPTR(cc_server_.get()).c_str()).c_str());

  global_api_ = make_unique<BackendGlobalApi>(*cc_server_.get(), bot_propg_set_);
  module_engine_ = make_unique<engine::ModuleEngine>(*global_api_);
  SET_DEBUG_TAG(*module_engine_, "bk_%p_modeng", this);

  auto& this_as_stopable(static_cast<Stopable&>(*this));
  tss_impl_ = make_unique<ThreadsafeStopableImpl>(this_as_stopable,
                                                  SeparatableObjects().tss_strand);
}

void BackendCore::AddModule(Uptr<engine::Module> module) {
  auto local_api(Uptr<BackendLocalApi>(new BackendLocalApi));
  module_engine_->AddModule(std::move(module), std::move(local_api));
}

// [LoopObject::PrepareToStart]
// AddModule cannot be called after PrepareToStart() !
void BackendCore::PrepareToStart(Errcode& err) {
  DCHECK(!prepared_);
  admin_server_->PrepareToStart(err);
  if (err) {
    return;
  }
  cc_server_->PrepareToStart(err);
  if (err) {
    return;
  }
  prepared_ = true;
  event_chain_head_.AttachTop(this);
  event_chain_head_.Freeze();
  CallModulesRegisterBotProperties();
}

void BackendCore::AllocBotModuleDataSlots(Shptr<cc::ICcBot> bot) {
  size_t modcount(module_engine_->GetModuleCount());

  // Every bot has BackendBotUserData. It contains data slots for all modules.
  auto bot_assoc_data = boost::make_shared<BackendBotUserData>(modcount);

  // Alloc slots for each module.
  for (size_t i = 0; i < modcount; i++) {
    engine::ModuleBase& module = module_engine_->GetModule(i);
    DCHECK(i == module.GetModuleIndex());
    Uptr<BackendBotModuleData> module_data;
    module_data = module.GetModuleApiAs<BackendModuleApi>().CreateBotModuleData();
    if (module_data) {
      //It's not yet connected to bot so nobody can racecond us.
      bot_assoc_data->SetBotModuleData(module.GetModuleIndex(),
                                       std::move(module_data));
    }
  }

  // Connect newly created data to the bot as CcUserData.
  // This call is threadsafe.
  bot->SetUserData(std::move(bot_assoc_data));
}

void BackendCore::CallModulesOnBotHandshake(Shptr<cc::ICcBot> bot) {
  size_t modcount(module_engine_->GetModuleCount());

  // Alloc slots for each module.
  for (size_t i = 0; i < modcount; i++) {
    engine::ModuleBase& module = module_engine_->GetModule(i);
    DCHECK(i == module.GetModuleIndex());
    module.GetModuleApiAs<BackendModuleApi>().OnBotHandshake(bot);
  }
}

Shptr<Session> BackendCore::AdminSessionFactoryFunc(Uptr<Stream> new_stm,
                                                    Shptr<Strand> sess_strand,
                                                    RefTrackerContextHandle rtctxhandle) {
  // inside admin server acceptor fiber
  // --------------------------------------------------------------------
  // Just use SAME strand
  // --------------------------------------------------------------------
  Shptr<Strand> taskmgr_strand(sess_strand);
  Shptr<Strand> jobmgr_strand(sess_strand);
  // ---
  auto cmd_rdr = SeparatableObjects().netshell_fac->CreateCommandReader(
    sess_strand, *new_stm.get());
  auto cmd_writ = SeparatableObjects().netshell_fac->CreateParallelCommandResultWriter(
    gZcaNsStatusDescriptorTable, sess_strand, *new_stm.get());
  auto& ioc_sess(admin_server_->GetAcceptorIoContext());

  // TODO: what's better for performance and resources:
  // to have +1 strand or use same strand for >1 object ?
  Uptr<TaskManager> new_taskmgr = make_unique<TaskManager>(taskmgr_strand,
                                                           &ioc_sess); // for work simulation
  SET_DEBUG_TAG(*new_taskmgr, "admin_%p_taskmgr", this);

  auto jobmgr_taskmgr(make_unique<TaskManager>(jobmgr_strand,
                                               &ioc_sess)); // for work simulation

  return make_shared<BackendAdminSession>(
    std::move(new_stm),
    sess_strand,
    module_engine_->CreateEngineSession(
        sess_strand,
        rtctxhandle,
        ioc_sess,
        move(new_taskmgr),
        make_unique<BasicJobManager>(move(jobmgr_taskmgr))
    ),
    std::move(cmd_rdr),
    std::move(cmd_writ)
    );
}

void BackendCore::CallModulesRegisterBotProperties() {
  const size_t modcount = module_engine_->GetModuleCount();
  for (size_t i = 0; i < modcount; i++) {
    engine::Module& module = module_engine_->GetModule(i);
    module.GetModuleApiAs<BackendModuleApi>()
      .RegisterBotProperties(bot_propg_set_reg_);
  }
}

// [CcServerEventsLink impl]
void BackendCore::OnBotHandshakeComplete(Shptr<cc::ICcBot> bot) {
  // We are a link that is always on top of event dispatch chain
  // First, alloc slots; |bot| won't be provided to modules
  AllocBotModuleDataSlots(bot);

  // Second, call every module's OnBotHandshake, providing |bot|
  CallModulesOnBotHandshake(bot);

  // Call next event dispatcher if exist
  co::FilterChainLink<cc::CcServerEvents>* next = CcServerEventsLink::GetNext();
  if (next) {
    next->OnBotHandshakeComplete(bot);
  }
}

void BackendCore::StopUnsafe() {
  llog() << "stopping admin server\n";
  admin_server_->StopThreadsafe();

  llog() << "stopping cc server \n";
  cc_server_->StopThreadsafe();
}

/************************************************************************
* BackendCore::Start()
*
************************************************************************/
void BackendCore::Start(RefTracker rt) {
  DCHECK(prepared_);

  tss_impl_->BeforeStartWithIoEnded(rt, rt);

  // class Server decide itself in which thread is to call AdminSessionFactoryFunc
  admin_server_->SetSessionFactoryFunc(
        co::bind(&BackendCore::AdminSessionFactoryFunc, this, _1, _2, rt.GetContextHandle()));

  RefTracker rt_all(CUR_LOC(), [&]() {
    llog() << " -= rt_all =-\n";
                            },
                     rt);

  // Every rt do StopThreadsafe to ensure everything is stopped

  RefTracker rt_servers(CUR_LOC(), [&]() {
    llog() << " ; rt_servers\n";
    // Server stopped which means all i/o ended. Now we can do whatever from any thread
    _dbg_servers_stopped_ = true;
    StopThreadsafe();
                        },
                        rt_all);

  RefTracker rt_admin_server(CUR_LOC(), [&]() {
    llog() << " ; rt_admin_server\n";
    _dbg_adminserver_stopped_ = true;
                             },
                             rt_servers);

  RefTracker rt_cc_server(CUR_LOC(), [&]() {
    llog() << " ; rt_cc_server\n";
    _dbg_ccserver_stopped_ = true;
                          },
                          rt_servers);

  llog() << "starting admin server\n";
  admin_server_->Start(rt_admin_server);

  llog() << "starting cc server\n";
  cc_server_->Start(rt_cc_server);

  // Can be module_engine_->InitSomething
}

void BackendCore::StopThreadsafe()
{
  llog() << "StopThreadsafe\n";
  tss_impl_->StopThreadsafe();
}

void BackendCore::CleanupAbortedStop() {
  admin_server_->CleanupAbortedStop();
  cc_server_->CleanupAbortedStop();
}

void BackendCore::GetLocalAddressToConnect(Endpoint& addr, Errcode& err) {
  admin_server_->GetLocalAddressToConnect(addr, err);
}

void BackendCore::GetLocalCcAddressToConnect(Endpoint& addr, Errcode& err) {
  cc_server_->GetLocalAddressToConnect(addr, err);
}


// -----------------------------------------------------------------------------------------------

core::CcServerEventsFilterChainHead& BackendCore::GetCcServerEventsFilterChain() {
  return event_chain_head_;
}


// -----------------------------------------------------------------------------------------------

// Exception throwing wrappers
Endpoint BackendCore::GetLocalAddressToConnect() {
  Errcode err;
  Endpoint addr;
  GetLocalAddressToConnect(addr, err);
  if (err) {
    throw boost::system::system_error(err);
  }
  return addr;
}
Endpoint BackendCore::GetLocalCcAddressToConnect() {
  Errcode err;
  Endpoint addr;
  GetLocalCcAddressToConnect(addr, err);
  if (err) {
    throw boost::system::system_error(err);
  }
  return addr;
}



}}
