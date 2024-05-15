#include "zca/core/front/frontend_core.h"
#include "zca/core/front/frontend_builder.h"
#include "zca/core/front/frontend_admin_session.h"

#include "zca/pki/create_pki_factory.h"

#include "zca/netshell_status_descriptor_table.h"

//#include "co/async/test_kit/test_session.h"
#include "co/async/create_for_endpoint.h"
#include "co/base/strings.h"
#include "co/xlog/xlog.h"

using namespace co;
using namespace co::async;
using namespace std;
using co::net::Endpoint;

namespace core {
namespace front {

DEFINE_XLOGGER_SINK("frontend", gFrontendLogSink);
#define XLOG_CURRENT_SINK gFrontendLogSink

#define llog() Log(_DBG) << "Bk " << SPTR(this) << " `" << "" << "` "

Uptr<FrontendBuilder::TSepObjects> FrontendBuilder::BuildSeparatableObjects(
  ThreadModel & tm,
  const FrontendConfig& conf,
  const FrontendSeparationConfig& sep)
{
  auto sepobjs = make_unique<FrontendBuildTypes::SeparatableObjects>();

  sepobjs->admin_server_acceptor = CreateStreamAcceptorForEndpoint(
    tm.AcquireIoContextForThreadGroup(sep.admin_acceptor_threadgroup),
    conf.bk_addr);

  SET_DEBUG_TAG(sepobjs->admin_server_acceptor->DebugTag(), "front_admin_srv_acpt");

  sepobjs->admin_stream_factory = CreateStreamFactoryForEndpoint(
    tm.AcquireIoContextForThreadGroup(sep.admin_sessions_threadgroup),
    conf.bk_addr);

  sepobjs->bk_ns_connector = CreateConnectorForEndpoint(conf.bk_addr);

  sepobjs->bk_stream_factory = CreateStreamFactoryForEndpoint(
    tm.AcquireIoContextForThreadGroup(sep.admin_sessions_threadgroup),
    conf.bk_addr);

  sepobjs->netshell_fac = make_unique<netshell::NsServerFactoryText>();

  sepobjs->bk_ns_factory = make_unique<netshell::NsClientFactoryText>();

  sepobjs->ioc_admin_sess_strands = &tm.AcquireIoContextForThreadGroup(
    sep.admin_sessions_threadgroup);

  sepobjs->ioc_module_strands = &tm.AcquireIoContextForThreadGroup(
    sep.modules_threadgroup);

  sepobjs->tss_strand = make_shared<Strand>(
    tm.AcquireIoContextForThreadGroup(
    sep.tss_threadgroup));

  return sepobjs;
}

Uptr<FrontendBuilder::TObjects> FrontendBuilder::BuildObjects(ThreadModel& tm,
                                                              const FrontendConfig& conf) {
  auto objs = make_unique<TObjects>();
  objs->pki_fac_ = pki::CreatePkiFactory(conf.pki_factory_name);
  if (objs->pki_fac_ == nullptr) {
    NOTREACHED();
  }
  return objs;
}

Uptr<FrontendBuilder::TParams> FrontendBuilder::BuildParams(const FrontendConfig& conf) {
  auto par = make_unique<TParams>();
  par->user_config = conf;
  return par;
}

// FrontendCore

FrontendCore::FrontendCore()
{
}

// [Buildable::CompleteBuild]
void FrontendCore::CompleteBuild() {

  // CREATE ADMIN SERVER
  admin_server_ = make_unique<ServerWithSessList>(
    Params().user_config.frontend_server_locaddr,
    ServerObjects(
      SeparatableObjects().admin_stream_factory,
      std::move(SeparatableObjects().admin_server_acceptor),
      *SeparatableObjects().ioc_admin_sess_strands));

  // Set debug tags
  SET_DEBUG_TAG(admin_server_->_DbgTag(), string_printf("front_admsrv_%s", SPTR(admin_server_.get()).c_str()).c_str());

  //global_api_ = make_unique<FrontendGlobalApi>();
  module_engine_ = make_unique<engine::ModuleEngine>(*global_api_);
  SET_DEBUG_TAG(*module_engine_, "fr_%p_modeng", this);

  auto& this_as_stopable(static_cast<Stopable&>(*this));
  tss_impl_ = make_unique<ThreadsafeStopableImpl>(this_as_stopable,
                                                  SeparatableObjects().tss_strand);
}

void FrontendCore::AddModule(Uptr<engine::Module> module) {
  auto local_api(Uptr<FrontendLocalApi>(new FrontendLocalApi));
  module_engine_->AddModule(std::move(module), std::move(local_api));
}

// [LoopObject::PrepareToStart]
// AddModule cannot be called after PrepareToStart() !
void FrontendCore::PrepareToStart(Errcode& err) {
  DCHECK(!prepared_);
  admin_server_->PrepareToStart(err);
  if (err) {
    return;
  }
  prepared_ = true;

}

Shptr<Session> FrontendCore::AdminSessionFactoryFunc(Uptr<Stream> new_stm,
                                                     Shptr<Strand> sess_strand,
                                                     RefTrackerContextHandle rtctxhandle) {
  // inside admin server acceptor fiber
  auto cmd_rdr = SeparatableObjects().netshell_fac->CreateCommandReader(sess_strand, *new_stm.get());
  auto cmd_writ = SeparatableObjects().netshell_fac->CreateCommandResultWriter(
    gZcaNsStatusDescriptorTable,
    sess_strand, *new_stm.get());
  auto& ioc_sess(admin_server_->GetAcceptorIoContext());
  
  // FrontendAdminSession will create NsParaCommandExecutor from objects:
  DCHECK(SeparatableObjects().bk_stream_factory != nullptr);
  auto bk_stm = SeparatableObjects().bk_stream_factory->CreateStream();
  auto bk_writ = SeparatableObjects().bk_ns_factory->CreateCommandWriter(
    sess_strand, *bk_stm);
  auto bk_para_rdr = SeparatableObjects().bk_ns_factory->CreateParallelCommandResultReader(
    gZcaNsStatusDescriptorTable,
    sess_strand, *bk_stm);

  // TODO: what's better for performance and resources:
  // to have +1 strand or use same strand for >1 object ?
  Uptr<TaskManager> new_taskmgr = make_unique<TaskManager>(sess_strand,
                                                           &ioc_sess); // for work simulation
  SET_DEBUG_TAG(*new_taskmgr, "fr_admin_%p_taskmgr", this);

  auto jobmgr_taskmgr(make_unique<TaskManager>(sess_strand,
                                               &ioc_sess)); // for work simulation

  Uptr<pki::SignatureCreator> sig_creator = Objects().pki_fac_->CreateSignatureCreator();

  Uptr<engine::EngineSession> new_eng_sess =
    module_engine_->CreateEngineSession(
      sess_strand,
      rtctxhandle,
      ioc_sess,
      move(new_taskmgr),
      make_unique<BasicJobManager>(move(jobmgr_taskmgr)));

  return make_shared<FrontendAdminSession>(
    std::move(new_stm),
    sess_strand,
    std::move(new_eng_sess),
    std::move(cmd_rdr),
    std::move(cmd_writ),
    std::move(bk_stm), // we already used it for bk_writ and bk_para_rdr, but we also need session to own it
    std::move(bk_writ),
    std::move(bk_para_rdr),
    SeparatableObjects().bk_ns_connector,
    Params().user_config.bk_addr,
    std::move(sig_creator)
    );
}



void FrontendCore::StopUnsafe() {
  llog() << "stopping admin server\n";
  admin_server_->StopThreadsafe();
}

/************************************************************************
* FrontendCore::Start()
*
************************************************************************/
void FrontendCore::Start(RefTracker rt) {
  DCHECK(prepared_);

  tss_impl_->BeforeStartWithIoEnded(rt, rt);

  // class Server decide itself in which thread is to call AdminSessionFactoryFunc
  admin_server_->SetSessionFactoryFunc(
        co::bind(&FrontendCore::AdminSessionFactoryFunc, this, _1, _2, rt.GetContextHandle()));

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

  llog() << "starting admin server\n";
  admin_server_->Start(rt_admin_server);
}

void FrontendCore::StopThreadsafe()
{
  llog() << "StopThreadsafe\n";
  tss_impl_->StopThreadsafe();
}

void FrontendCore::CleanupAbortedStop() {
  admin_server_->CleanupAbortedStop();
}

void FrontendCore::GetLocalAddressToConnect(Endpoint& addr, Errcode& err) {
  admin_server_->GetLocalAddressToConnect(addr, err);
}

// -----------------------------------------------------------------------------------------------

// Exception throwing wrappers
Endpoint FrontendCore::GetLocalAddressToConnect() {
  Errcode err;
  Endpoint addr;
  GetLocalAddressToConnect(addr, err);
  if (err) {
    throw boost::system::system_error(err);
  }
  return addr;
}

}}
