
#include "zca/core/front/frontend_admin_session.h"

#include "zca/engine/sdk/job_manager.h"
#include "zca/engine/sdk/cmd_executor.h"

#include "zca/netshell_status_descriptor_table.h"


#include "co/async/wrap_post.h"
#include "co/base/strings.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;
using namespace netshell;
using namespace engine;

#define llog() Log(_DBG) << "FrontAdminSess (base)" << \
    SPTR(static_cast<co::async::SessionBase*>(this)) << " "

namespace core {
namespace front {
namespace {
  static NsCmdResult ResWithMsgBody(int retcode, const string& msg) {
    return NsCmdResult(kNsCmdExecuted, retcode, NsResultType::kMessage)
      .WithMessageBody(msg);
  }
  }
atomic<uint64_t> _dbgNumFrontAdminSessions = 0;
atomic<uint64_t> _dbgNumFrontAdminSessionsIoStarted = 0;
atomic<uint64_t> _dbgNumFrontAdminSessionsIoEnded = 0;

DEFINE_XLOGGER_SINK("fradmsess", gFrontendAdminSessionSink);
#define XLOG_CURRENT_SINK gFrontendAdminSessionSink

FrontendAdminSession::FrontendAdminSession(
    Uptr<Stream> new_stm,
    Shptr<Strand> strand,
    Uptr<engine::EngineSession> engine_session,
    Uptr<netshell::NsCommandReader> adm_netshell_rdr,
    Uptr<netshell::NsCommandResultWriter> adm_netshell_writ,
    Uptr<Stream> bk_stm,
    Uptr<NsCommandWriter> bk_writ,
    Uptr<NsParaCommandResultReader> bk_para_rdr,
    Shptr<StreamConnector> bk_stm_connector,
    Endpoint backend_addr,
    Uptr<pki::SignatureCreator> sig_creator)
  :
  Session(std::move(new_stm), strand),
  engine_session_(std::move(engine_session)),
  adm_netshell_rdr_(std::move(adm_netshell_rdr)),
  adm_netshell_writ_(std::move(adm_netshell_writ)),
  bk_stm_(std::move(bk_stm)),
  bk_writ_(std::move(bk_writ)),
  bk_para_rdr_(std::move(bk_para_rdr)),
  bk_para_executor_(
    make_unique<NsParaCommandExecutor>(
      *bk_writ_.get(),
      *bk_para_rdr_.get(),
      strand)),
  bk_stm_connector_(bk_stm_connector),
  backend_addr_(backend_addr),
  sig_creator_(std::move(sig_creator))
{
  _dbgNumFrontAdminSessions += 1;
  llog() << "CTOR\n";
}

FrontendAdminSession::~FrontendAdminSession() {
  _dbgNumFrontAdminSessions -= 1;
  llog() << "~~~DTOR~~~\n";
}

void FrontendAdminSession::StopUnsafe() {
  // Closes taskmgr(s) in engine session, etc.
  engine_session_->StopThreadsafe();
  // This closes stream
  Session::StopUnsafe();
}

void FrontendAdminSession::CleanupAbortedStop() {
  engine_session_->CleanupAbortedStop();
  
  adm_netshell_rdr_->CleanupAbortedStop();
  adm_netshell_writ_->CleanupAbortedStop();
  // Cannot cancel connect. On asio level, our connector calls just asio socket's async_connect(). To cancel connects, we should use cancel() // https://live.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/ip__tcp/socket.html   // But this would cancel any other operations so it's not good idea to implement such    // total cancel just for connector cancel. Leave without cancel.
  //bk_stm_connector_->CancelConnectSomehow();
}

netshell::NsParaCommandExecutor& FrontendAdminSession::GetBackshellParaExecutor() {
  return *bk_para_executor_.get();
}

pki::SignatureCreator& FrontendAdminSession::GetSignatureCreator() {
  return *sig_creator_.get();
}


void FrontendAdminSession::BeginIo(RefTracker rt) {
  llog() << "BeginIo\n";

  rt.SetReferencedObject(shared_from_this());

  _dbgNumFrontAdminSessionsIoStarted += 1;
  RefTracker rt_all(CUR_LOC(), [this]() {
    llog() << " ; rt_all\n";
    _dbgNumFrontAdminSessionsIoEnded += 1;
                    },
                    rt);

  // create module data slots
  engine_session_->InitializeSlots();

  engine_session_->SetCustomApi(this);

  RefTracker rt_engsess(CUR_LOC(), [&]() {
    // inside unknown fiber,  not synchronized with #3
    llog() << " ; rt_engsess [died]\n";
    // On any Stop, Stop other objects
    StopThreadsafe();
                        },
                        rt_all);

  engine_session_->Start(rt_engsess);


  bk_stm_connector_->AsyncConnect(backend_addr_, *bk_stm_.get(),
    wrap_post(GetFiberStrand(),
      co::bind(&FrontendAdminSession::HandleConnectToBackendParallelNetshell,
        shared_from_this(), _1, rt)));
}

void FrontendAdminSession::HandleConnectToBackendParallelNetshell(
  Errcode err, 
  RefTracker rt) 
{
  if (err) {
    // Further commands from admin won't be executed
    back_conn_err_ = err;
  }
  
  ReadCommandsAgain(RefTracker(CUR_LOC(), [this]() {
    // inside unknown fiber,  not synchronized with #2
    llog() << " netshell i/o ended\n";
    // On any Stop, Stop other objects
    StopThreadsafe();
                    },
                    rt));
}

void FrontendAdminSession::ReadCommandsAgain(RefTracker rt) {
  adm_netshell_rdr_->AsyncReadCommand(
    cur_read_cmd_,
    wrap_post(GetFiberStrand(),
      co::bind(&FrontendAdminSession::HandleReadCommand,
        shared_from_this(), _1, rt)));
}

void FrontendAdminSession::HandleReadCommand(Errcode err, RefTracker rt) {
  DCHECK(IsInsideFiberStrand());
  if (err) {
    return;
  }
  if (back_conn_err_) {
    // Instead of executing command, reply with error message
    cur_cmd_result_ = ResWithMsgBody(-1,
      string_printf("backend connect was unsuccessful (err %s)", back_conn_err_.message().c_str()));
    WriteResult(rt);
    return;
  }

  // Backend connection was successfull so we fall to ExecuteCommand
  Shptr<Task> task_spawned;
  engine_session_->ExecuteCommand(
    nullptr, // input_ns_result
    cur_read_cmd_,
    cur_cmd_result_,
    wrap_post(GetFiberStrand(),
      co::bind(&FrontendAdminSession::HandleExecuteCommand, shared_from_this(),
        rt)), // Carrying cmd and result args
    task_spawned);
}

void FrontendAdminSession::HandleExecuteCommand(RefTracker rt) {
  DCHECK(IsInsideFiberStrand());
  WriteResult(rt);
}

void FrontendAdminSession::WriteResult(RefTracker rt) {
  DCHECK(IsInsideFiberStrand());
  adm_netshell_writ_->AsyncWriteResult(
    cur_cmd_result_,
    wrap_post(GetFiberStrand(), co::bind(&FrontendAdminSession::HandleWriteResult,
      shared_from_this(), _1, _2, rt)));
}

void FrontendAdminSession::HandleWriteResult(
  Errcode err, size_t bytes_written, RefTracker rt)
{
  DCHECK(IsInsideFiberStrand());
  if (err) {
    return;
  }
  ReadCommandsAgain(rt);
}

}}






































