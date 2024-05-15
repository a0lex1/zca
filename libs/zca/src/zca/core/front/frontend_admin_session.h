#pragma once

#include "zca/core/front/admin_session_custom_api.h"
#include "zca/engine/sdk/module_set.h"
#include "zca/engine/sdk/engine_session.h"

#include "netshell/netshell_factory.h"
#include "netshell/ns_para_command_executor.h"

#include "co/async/task_manager.h"
#include "co/async/session.h"
#include "co/async/stream_connector.h"

#include "co/xlog/define_logger_sink.h"

#include <atomic>

namespace core {
namespace front {

DECLARE_XLOGGER_SINK("fradmsess", gFrontendAdminSessionSink);

extern std::atomic<uint64_t> _dbgNumFrontAdminSessions;
extern std::atomic<uint64_t> _dbgNumFrontAdminSessionsIoStarted;
extern std::atomic<uint64_t> _dbgNumFrontAdminSessionsIoEnded;

// class AdminSession
//
// Aggregates module engine's EngineSession object.
class FrontendAdminSession :
  public co::enable_shared_from_this<FrontendAdminSession>,
  public co::async::Session,
  private AdminSessionCustomApi
{
public:
  virtual ~FrontendAdminSession();

  using RefTracker = co::RefTracker;
  using Endpoint = co::net::Endpoint;
  using Stream = co::async::Stream;
  using StreamConnector = co::async::StreamConnector;

  // |cmd_handler| can be called directly without posting
  FrontendAdminSession(
    Uptr<Stream> new_stm,
    Shptr<Strand> strand,
    Uptr<engine::EngineSession> engine_session,
    Uptr<netshell::NsCommandReader> adm_netshell_rdr,
    Uptr<netshell::NsCommandResultWriter> adm_netshell_writ,
    Uptr<Stream> bk_stm,
    Uptr<netshell::NsCommandWriter> bk_writ,
    Uptr<netshell::NsParaCommandResultReader> br_para_rdr,
    Shptr<StreamConnector> bk_stm_connector,
    Endpoint backend_addr,
    Uptr<pki::SignatureCreator> sig_creator);

private:
  void BeginIo(RefTracker rt) override;
  void StopUnsafe() override;
  void CleanupAbortedStop() override;

  void HandleConnectToBackendParallelNetshell(Errcode err, RefTracker rt);
  void ReadCommandsAgain(RefTracker);
  void HandleReadCommand(Errcode, RefTracker);
  void HandleExecuteCommand(RefTracker);
  void HandleWriteResult(Errcode, size_t bytes_written, RefTracker);
  void WriteResult(RefTracker);

private:
  // [AdminSessionCustomApi impl]
  netshell::NsParaCommandExecutor& GetBackshellParaExecutor() override;
  pki::SignatureCreator& GetSignatureCreator() override;
private:
  Uptr<pki::SignatureCreator> sig_creator_;
  uint64_t glob_cmd_index_{ 0 };

  Uptr<engine::EngineSession> engine_session_; // need EngineSession::Start cuz we have TaskManager::Start
  Uptr<netshell::NsCommandReader> adm_netshell_rdr_; 
  Uptr<netshell::NsCommandResultWriter> adm_netshell_writ_; 
  Uptr<Stream> bk_stm_;
  
  Uptr<netshell::NsCommandWriter> bk_writ_;
  Uptr<netshell::NsParaCommandResultReader> bk_para_rdr_;
  Uptr<netshell::NsParaCommandExecutor> bk_para_executor_; // uses previous two
  Shptr<StreamConnector> bk_stm_connector_;
  
  Endpoint backend_addr_;

  Errcode back_conn_err_; // if none-null, we reply with `not connected to backend` status
  std::string cur_read_cmd_;
  netshell::NsCmdResult cur_cmd_result_;
};

}}

