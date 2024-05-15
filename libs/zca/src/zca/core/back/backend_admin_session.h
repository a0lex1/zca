#pragma once

#include "zca/engine/sdk/module_set.h"
#include "zca/engine/sdk/engine_session.h"

#include "netshell/netshell_factory.h"
#include "netshell/ns_para_command_result_writer_queue_st.h"

#include "co/async/task_manager.h"
#include "co/async/session.h"

#include "co/xlog/define_logger_sink.h"

#include <atomic>

namespace core {
namespace back {

DECLARE_XLOGGER_SINK("bkadmsess", gBackendAdminSessionSink);

extern std::atomic<uint64_t> _dbgNumBackAdminSessions;
extern std::atomic<uint64_t> _dbgNumBackAdminSessionsIoStarted;
extern std::atomic<uint64_t> _dbgNumBackAdminSessionsIoEnded;

// class AdminSession
//
// Aggregates module engine's EngineSession object.
//
class BackendAdminSession :
  public co::enable_shared_from_this<BackendAdminSession>,
  public co::async::Session
{
public:
  virtual ~BackendAdminSession();

  using RefTracker = co::RefTracker;
  using Stream = co::async::Stream;

  // |cmd_handler| can be called directly without posting
  BackendAdminSession(Uptr<Stream> new_stm,
    Shptr<Strand> strand,
    Uptr<engine::EngineSession> engine_session,
    Uptr<netshell::NsCommandReader> netshell_rdr,
    Uptr<netshell::NsParaCommandResultWriter> netshell_writ);

private:
  void BeginIo(RefTracker rt) override;
  void StopUnsafe() override;
  void CleanupAbortedStop() override;

  void ReadCommandsAgain(RefTracker);
  void HandleReadCommand(Errcode, RefTracker);
  void HandleExecuteCommand(uint64_t cmd_index, std::string cmd, Shptr<netshell::NsCmdResult>, RefTracker);
  void HandleWriteResult(Errcode, size_t bytes_written, uint64_t, std::string, Shptr<netshell::NsCmdResult>, RefTracker);

private:
  uint64_t glob_cmd_index_{ 0 };
  Uptr<engine::EngineSession> engine_session_; // need EngineSession::Start cuz we have TaskManager::Start
  Uptr<netshell::NsCommandReader> netshell_rdr_;
  Uptr<netshell::NsParaCommandResultWriter> par_netshell_res_writ_;
  Uptr<netshell::NsParaCommandResultWriterQueueST> par_netshell_res_writ_qst_;
  std::string cur_read_cmd_;
  netshell::NsCmdResult cur_cmd_result_;
};

}}

