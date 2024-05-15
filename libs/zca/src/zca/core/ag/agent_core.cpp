#include "zca/core/ag/agent_core.h"
#include "zca/core/ag/agent_builder.h"
#include "zca/core/cmd_opaque_data.h"
#include "zca/pki/create_pki_factory.h"

#include "zca/agent_config.h"
#include "zca/netshell_status_descriptor_table.h"

#include "netshell/get_serialized.h"
#include "netshell/serializer.h"

#include "co/async/create_for_endpoint.h"
#include "co/async/test_kit/test_session.h"

#include "co/base/base64.h"
#include "co/base/strings.h"
#include "co/base/bin_reader.h"
#include "co/base/bin_writer.h"
#include "co/xlog/xlog.h"

using namespace co;
using namespace co::async;
using namespace netshell;
using namespace std;
using namespace cc;
using namespace boost::posix_time;
using boost::asio::deadline_timer;

#define llog() Log(_DBG) << "Ag " << SPTR(this) << " `" << GET_DEBUG_TAG(*this) << "` "

namespace core {
namespace ag {

DEFINE_XLOGGER_SINK("agent", gAgentLogSink);
#define XLOG_CURRENT_SINK gAgentLogSink

Uptr<AgentBuilder::TSepObjects> AgentBuilder::BuildSeparatableObjects(
  ThreadModel & tm,
  const AgentConfig& conf,
  const AgentSeparationConfig&)
{
  auto sepobjs = make_unique<AgentBuildTypes::SeparatableObjects>();

  sepobjs->ioc_eng_sess = &tm.DefIOC();
  sepobjs->client_stream = CreateStreamForEndpoint(tm.DefIOC(),
                                                   conf.back_remaddr);

  auto mkstrand = [&]() {
    return make_shared<Strand>(tm.DefIOC());
  };

  sepobjs->cc_session_strand = mkstrand();
  sepobjs->taskmgr_strand = mkstrand();
  sepobjs->jobmgr_strand = mkstrand();
  sepobjs->tss_strand = mkstrand();
  sepobjs->engsess_strand = mkstrand();

  return sepobjs;
}

Uptr<AgentBuilder::TObjects> AgentBuilder::BuildObjects(
  ThreadModel&,
  const AgentConfig& conf)
{
  auto objs = make_unique<TObjects>();
  
  objs->pki_fac_ = pki::CreatePkiFactory(conf.pki_factory_name);
  if (objs->pki_fac_ == nullptr) {
    NOTREACHED();
  }

  return objs;
}

Uptr<AgentBuilder::TParams> AgentBuilder::BuildParams(const AgentConfig& conf) {
  auto par = make_unique<TParams>();
  // Copy original config
  par->user_config = conf;
  return par;
}

AgentCore::~AgentCore() {
  llog() << "~~~DTOR~~~\n";
}

AgentCore::AgentCore()
{
  llog() << "CTOR\n";
}

void AgentCore::SetBackendCcRemoteAddr(const Endpoint& new_addr) {
  // co::async::Client::SetRemoteAddress
  cc_client_->SetRemoteAddress(new_addr);
}

// [Buildable::CompleteBuild]
void AgentCore::CompleteBuild() {
  const auto& bot_id(Params().user_config.bot_id);

  llog() << "using bot opts { botid=" << bot_id.ToStringRepr() << " }\n";
  CcClientBotOptions bot_opts(
    bot_id,
    Params().user_config.hshake_postpone_enable,
    Params().user_config.hshake_postpone_delay);

  auto backend_remaddr(Params().user_config.back_remaddr);
  auto dummy_connector(CreateConnectorForEndpoint(backend_remaddr));
  Uptr<StreamConnector> connector(move(dummy_connector));


  cc_client_ = make_unique<CcClient>(backend_remaddr,
                                     move(connector),
                                     move(SeparatableObjects().client_stream),
                                     SeparatableObjects().cc_session_strand,
                                     static_cast<CcClientCommandDispatcher&>(*this),
                                     &event_chain_head_,
                                     Params().user_config.max_chunk_body_size,
                                     bot_opts);
  if (Params().user_config.ping_interval.is_zero()) {
    cc_client_->UseServerPingInterval();
  }
  else {
    cc_client_->SetPingInterval(Params().user_config.ping_interval);
  }
  cc_client_->SetTrafficEncryptionKeys("options", 6, "unique", 6);

  global_api_ = Uptr<AgentGlobalApi>(new AgentGlobalApi(*cc_client_.get()));
  module_engine_ = make_unique<engine::ModuleEngine>(*global_api_.get());
  SET_DEBUG_TAG(*module_engine_, "ag_%p_modeng", this);

  salt_generator_ = Objects().pki_fac_->CreateSaltGenerator();
  sig_verifier = Objects().pki_fac_->CreateSignatureVerifier();

  auto& this_as_stopable(static_cast<Stopable&>(*this));
  tss_impl_ = make_unique<ThreadsafeStopableImpl>(this_as_stopable,
                                                  SeparatableObjects().tss_strand);
}

void AgentCore::AddModule(Uptr<engine::Module> module) {
  auto local_api(Uptr<AgentLocalApi>(new AgentLocalApi));
  module_engine_->AddModule(std::move(module), std::move(local_api));
}

// [LoopObject::PrepareToStart]
void AgentCore::PrepareToStart(Errcode& err) {
  // We can't create module_engine_ here because it needs RefTrackerContextHandle
  // which is known only from Start().
  salt_generator_->GenerateSalt(current_salt_);
  cc_client_->SetOpaqueHandshakeData(make_unique<string>(current_salt_));
  cc_client_->PrepareToStart(err);
  bools_.prepared_to_start_ = true;
}


string AgentCore::SerializedResultWithEmptySalt(const NsCmdResult& res) {
  string buf = GetSerializedNsCmdResult(gZcaNsStatusDescriptorTable, res);
  BinWriter writer(buf);
  writer.WriteString("");
  return buf;
}


// Dispatch CC Command
void AgentCore::DispatchCommand(Uptr<std::string> cmd_opaque_data,
                                std::string& cmd_result_opaque_data,
                                EmptyHandler handler)
{
  // INSIDE CC UNKNOWN FIBER

  CmdOpaqueData opaque_data;
  BinReader reader(*cmd_opaque_data);
  if (!opaque_data.Unserialize(reader)) {
    // bot gave us incorrect bytes
    cmd_result_opaque_data = SerializedResultWithEmptySalt(NsCmdResult(kNsCmdExecuted, -1)
      .WithMessageBody("Malformed cmd opaque data"));
    handler();
    return;
  }
  if (opaque_data.signature.length()) {
    llog() << "Got Command: " << opaque_data.cmdline << "\n" << "SIGNATURE: " << opaque_data.signature << "\n";
  }
  else {
    llog() << "Got Command *WITHOUT SIGNATURE*: " << opaque_data.cmdline << "\n";
  }
  if (!Params().user_config.disable_cmd_sig_check) {
    if (!opaque_data.signature.length()) {
      cmd_result_opaque_data = SerializedResultWithEmptySalt(NsCmdResult(kNsCmdExecuted, -1)
        .WithMessageBody("Need ^^"));
      handler();
      return;
    }
    // concat everything as a string
    // another side code in zca/modules/basecmd/back/cmdexec_cmd_task.cpp
    string fruitsalad = string_to_upper(GetBotId().ToStringRepr())
      + current_salt_
      + opaque_data.cmdline;

    // Ignoring `can't decode` errors here (default fallback string)
    string decoded_opaque_sig = co::decode64(opaque_data.signature);
    bool is_sig_valid;
    sig_verifier->VerifySignature(
      fruitsalad,
      decoded_opaque_sig,
      is_sig_valid);

    if (!is_sig_valid) {
      cmd_result_opaque_data = SerializedResultWithEmptySalt(NsCmdResult(kNsCmdExecuted, -1)
        .WithMessageBody("Bad ^^"));
      handler();
      return;
    }

  }

  user_cmd_result_opaque_data_ = &cmd_result_opaque_data;
  user_handler_ = handler;
  cmd_result_ = make_unique<NsCmdResult>();

  Shptr<Task> ignored_task_spawned;
  engine_session_->ExecuteCommand(
    nullptr, // input_ns_result
    opaque_data.cmdline,
    *cmd_result_.get(),
    wrap_post(*GetTssStrand().get(),
              co::bind(&AgentCore::HandleExecuteCommand, this)),
   ignored_task_spawned);
}

// === other end zca/modules/basecmd/back/cmdexec_cmd_task.cpp ===

void AgentCore::HandleExecuteCommand() {
  DCHECK(IsInsideTssStrand());
  DCHECK(user_cmd_result_opaque_data_);

  // move to user result
  co::BinWriter writer(*user_cmd_result_opaque_data_);
  NsCmdResultSerializer serer(gZcaNsStatusDescriptorTable, *cmd_result_);
  serer.Serialize(writer);
  // change salt and append new salt to binary buffer
  salt_generator_->GenerateSalt(current_salt_); // change salt locally
  writer.WriteString(current_salt_);

  // explicitly clear to help debugging
  cmd_result_ = nullptr;
  user_cmd_result_opaque_data_ = nullptr;

  // Can call directly.
  user_handler_();
  user_handler_ = nullptr;
}

void AgentCore::StopUnsafe() {
  // We are called synchronizgly (we are StopUnsafe()) and we're calling
  // StopThreadsafe methods, it's normal

  llog() << "StopUnsafe\n";

  llog() << "stopping CcClient " << SPTR(cc_client_.get()) << "\n";
  cc_client_->StopThreadsafe();

  llog() << "stopping engine session " << SPTR(cc_client_.get()) << "\n";
  engine_session_->StopThreadsafe();
}

/***********************************************************************
* AgentCore::Start()
*
************************************************************************/
void AgentCore::Start(RefTracker rt) {
  DCHECK(bools_.prepared_to_start_);
  DCHECK(!bools_.started_);

  tss_impl_->BeforeStartWithIoEnded(rt, rt);

  bools_.started_ = true;

  RefTracker rt_all(CUR_LOC(), [&]() {
    llog() << "-= rt_all -= #ioended\n";
                        },
                        rt);

  tss_impl_->DoThreadsafeStart([&, rt_all] () {
    DCHECK(IsInsideTssStrand());
    StartObjects(rt_all);
  });
}

void AgentCore::StartObjects(RefTracker _rt) {
  using engine::BasicJobManager;

  // We now know user's |_rt| so we can use its context handle for engine_session_ (it's used for taskmgr inside it)
  llog() << "Starting objects: creating module engine and session\n";

  // Create objects for EngineSession and EngineSession itself
  auto new_taskmgr(make_unique<TaskManager>(SeparatableObjects().taskmgr_strand,
                                            SeparatableObjects().ioc_eng_sess)); // for work simulation
  SET_DEBUG_TAG(*new_taskmgr, "ag_%p_engsess_taskmgr", this);

  auto jobmgr_taskmgr(make_unique<TaskManager>(SeparatableObjects().jobmgr_strand,
                                               SeparatableObjects().ioc_eng_sess)); // for work simulation
  SET_DEBUG_TAG(*jobmgr_taskmgr, "ag_%p_jobmgr_taskmgr", this);

  engine_session_ = module_engine_->CreateEngineSession(
    SeparatableObjects().engsess_strand,
    _rt.GetContextHandle(), // engine session rtctx handle (used for taskmgr, )
    *SeparatableObjects().ioc_eng_sess,
    move(new_taskmgr),
    make_unique<BasicJobManager>(move(jobmgr_taskmgr))
    );

  llog() << "Starting objects: initializing slots\n";
  engine_session_->InitializeSlots();

  llog() << "Starting objects: staring |engine_session_|\n";
  RefTracker rt_engsess(CUR_LOC(), [&]() {
    // INSIDE UNKNOWN FIBER
    llog() << " ; rt_engsess\n";
    _dbg_engsess_stopped_ = true;
    StopThreadsafe();
                        },
                        _rt);
  engine_session_->Start(rt_engsess);

  RefTracker rt_client(CUR_LOC(),
                       [&]() {
    // INSIDE UNKNOWN FIBER
    llog() << " ; rt_client, connerr="<< cc_client_->GetConnectError()<<"\n";

    _dbg_client_stopped_ = true;
    StopThreadsafe();
                       },
                       _rt);

  llog() << "Starting objects: starting cc_client_\n";

  cc_client_->Start(rt_client);
}

void AgentCore::StopThreadsafe() {
  llog() << "StopThreadsafe\n";
  tss_impl_->StopThreadsafe();
}

void AgentCore::CleanupAbortedStop() {
  llog() << "CleanupAbortedStop\n";
  cc_client_->CleanupAbortedStop();
  engine_session_->CleanupAbortedStop();
}

core::CcClientEventsFilterChainHead& AgentCore::GetCcClientEventsFilterChain() {
  return event_chain_head_;
}

const cc::CcClientBotOptions& AgentCore::GetClientBotOptions() const {
  return cc_client_->GetBotOptions();
}

}}
