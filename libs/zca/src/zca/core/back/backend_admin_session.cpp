#include "zca/core/back/backend_admin_session.h"

#include "zca/engine/sdk/job_manager.h"
#include "zca/engine/sdk/cmd_executor.h"

#include "co/async/wrap_post.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;
using namespace netshell;
using namespace engine;

#define llogt() Log(_TRACE) << "BackAdminSess (base)" << SPTR(static_cast<co::async::SessionBase*>(this)) << " "
#define llog() Log(_DBG) << "BackAdminSess (base)" << SPTR(static_cast<co::async::SessionBase*>(this)) << " "

namespace core {
namespace back {

atomic<uint64_t> _dbgNumAdminSessions = 0;
atomic<uint64_t> _dbgNumAdminSessionsIoStarted = 0;
atomic<uint64_t> _dbgNumAdminSessionsIoEnded = 0;

DEFINE_XLOGGER_SINK("bkadmsess", gBackendAdminSessionSink);
#define XLOG_CURRENT_SINK gBackendAdminSessionSink

// This is BACKEND admin session
BackendAdminSession::BackendAdminSession(Uptr<Stream> new_stm,
  Shptr<Strand> _strand,
  Uptr<EngineSession> engine_session,
  Uptr<NsCommandReader> netshell_rdr,
  Uptr<NsParaCommandResultWriter> netshell_writ)
  :
  Session(std::move(new_stm), _strand),
  engine_session_(move(engine_session)),
  netshell_rdr_(std::move(netshell_rdr)),
  par_netshell_res_writ_(std::move(netshell_writ)),
  par_netshell_res_writ_qst_(make_unique<NsParaCommandResultWriterQueueST>(*par_netshell_res_writ_))
{
  _dbgNumAdminSessions += 1;
  llog() << "CTOR\n";
}

BackendAdminSession::~BackendAdminSession() {
  _dbgNumAdminSessions -= 1;
  llog() << "~~~DTOR~~~\n";
}

void BackendAdminSession::StopUnsafe() {
  // Closes taskmgr(s) in engine session, etc.
  engine_session_->StopThreadsafe();
  // This closes stream
  Session::StopUnsafe();
}

void BackendAdminSession::CleanupAbortedStop() {
  engine_session_->CleanupAbortedStop();

  netshell_rdr_->CleanupAbortedStop();
  par_netshell_res_writ_qst_->CleanupAbortedStop();
}

void BackendAdminSession::BeginIo(RefTracker rt) {
  llog() << "BeginIo\n";
  rt.SetReferencedObject(shared_from_this());

  _dbgNumAdminSessionsIoStarted += 1;
  RefTracker rt_all(CUR_LOC(), [this]() {
    llog() << " ; rt_all\n";
    _dbgNumAdminSessionsIoEnded += 1;
                    },
                    rt);

  // create module data slots
  engine_session_->InitializeSlots();

  RefTracker rt_engsess(CUR_LOC(), [&]() {
    // inside unknown fiber,  not synchronized with #3
    llog() << " ; rt_engsess [died]\n";
    // On any Stop, Stop other objects
    StopThreadsafe();
                        },
                        rt_all);

  engine_session_->Start(rt_engsess);

  ReadCommandsAgain(RefTracker(CUR_LOC(), [this]() {
    // inside unknown fiber,  not synchronized with #2
    llog() << " netshell i/o ended\n";
    // On any Stop, Stop other objects
    StopThreadsafe();
                    },
                    rt_all));
}

void BackendAdminSession::ReadCommandsAgain(RefTracker rt) {
  netshell_rdr_->AsyncReadCommand(cur_read_cmd_,
    wrap_post(GetFiberStrand(),
      co::bind(&BackendAdminSession::HandleReadCommand,
        shared_from_this(), _1, rt)));
}

void BackendAdminSession::HandleReadCommand(Errcode err, RefTracker rt) {
  DCHECK(IsInsideFiberStrand());
  if (err) {
    // This is an error on the 'main' chain of async operations, we just need to abandon |rt| here, the session
    // will stop becayse we passed a special RefTracker here (see how we called ReadCommandsAgain).
    return;
  }
  // We don't use task returned because task manager will stop all tasks if needed
  Shptr<Task> task_spawned;
  // Execute all the command in parallel

  auto cmd_result = make_shared<NsCmdResult>();

  // We're branching operation chain here. If there will be a stop, underlying task manager
  // will stop all tasks (See TaskManager::StopUnsafe), so this branched commands (which are tasks internally)
  // will be stopped too.
  uint64_t cur_cmd_index = glob_cmd_index_;
  llogt() << "Executing command [" << glob_cmd_index_ << "]: " << cur_read_cmd_ << "\n";
  ++glob_cmd_index_;
  engine_session_->ExecuteCommand(
    nullptr, // input_ns_result
    cur_read_cmd_,
    *cmd_result.get(),
    wrap_post(GetFiberStrand(),
      co::bind(&BackendAdminSession::HandleExecuteCommand, shared_from_this(),
        cur_cmd_index, cur_read_cmd_, cmd_result, rt)), // Carrying cmd and result args
    task_spawned);


  // Don't wait for cmd execution, read cmds again right now
  ReadCommandsAgain(rt);
}


void BackendAdminSession::HandleExecuteCommand(
  uint64_t cmd_index, std::string cmd, Shptr<NsCmdResult> cmd_result, RefTracker rt)
{
  DCHECK(IsInsideFiberStrand()); // IMPORTANT: parallel with ReadCommandsAgain()

  llogt() << "Command [" << cmd_index << "] executed, writing result\n";

  // <<< QUEUED WRITE >>>
  par_netshell_res_writ_qst_->AsyncWriteParallelResult(cmd_index, *cmd_result,
    wrap_post(GetFiberStrand(), co::bind(&BackendAdminSession::HandleWriteResult,
    shared_from_this(), _1, _2, cmd_index, cmd, cmd_result, rt)));
}

void BackendAdminSession::HandleWriteResult(
  Errcode err, size_t bytes_written, uint64_t cmd_index, std::string cmd,
  Shptr<NsCmdResult> cmd_result, RefTracker rt)
{
  DCHECK(IsInsideFiberStrand()); // IMPORTANT: parallel with ReadCommandsAgain()

  llogt() << "Command [" << cmd_index << "] result written\n";

  // Nothing to do here. Commands are dispatched in background now.
  if (err) {
    // We possibly have several WriteResult operations so abandoning one of those |rt|s won't lead to the termination.
    // So we need to terminate the session explicitly.
    StopThreadsafe();
    return;
  }
}

}}

















