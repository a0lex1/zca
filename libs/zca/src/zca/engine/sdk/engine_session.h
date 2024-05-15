#pragma once

#include "zca/engine/sdk/engine_session_core.h"
#include "zca/engine/sdk/job_manager.h"

#include "netshell/ns_cmd_result.h"

#include "co/async/cleanupable.h"
#include "co/async/task_manager.h"

namespace engine {

class ModuleEngine;
class ModuleSet;
class SessionModuleData;
class TaskManager;
class JobManager;
class CmdExecutor;
class ModuleDataSlotVector;

class CustomApi {
public:
  virtual ~CustomApi() = default;
};

class EngineSession
  :
  public co::async::Fibered,
  public co::async::Startable,
  public co::async::ThreadsafeStopable,
  public co::async::Cleanupable,
  public EngineSessionCore,
  private EngineSessionApi,
  private EngineSessionInfo
{
public:
  virtual ~EngineSession() = default;

  using RefTrackerContext = co::RefTrackerContext;
  using RefTrackerContextHandle = co::RefTrackerContextHandle;
  using TaskManager = co::async::TaskManager;
  using Task = co::async::Task;
  
  // probably hide
  void InitializeSlots();
  void SetCustomApi(CustomApi* cua) { custom_api_ = cua; }

  void Start(co::RefTracker rt) override;
  void StopThreadsafe() override;

  void ExecuteCommand(
    const netshell::NsCmdResult* input_ns_result,
    const std::string& raw_cmd,
    netshell::NsCmdResult& ns_result,
    EmptyHandler handler,
    Shptr<Task> task_spawned);

  void CleanupAbortedStop() override {
    task_mgr_->CleanupAbortedStop();
    job_mgr_->CleanupAbortedStop();
  }

  // [SessionCore impl]
  EngineSessionApi& GetSessionApi() override;
  SessionModuleData* GetSessionModuleDataFor(size_t module_idx) override;

  // [SessionApi impl]
  const EngineSessionInfo& GetSessionInfo() override;
  io_context& GetSessionIoContext() override;
  void KillSession() override;
  EngineSessionCore& GetSessionCore() override;

  // [SessionInfo impl]
  int GetSessionId() const override;

  using ModuleDataSlotVector = std::vector<Uptr<SessionModuleData>>;

private:
  friend class ModuleEngine;
  EngineSession(Shptr<Strand> strand,
                io_context& ioc,
                int session_id,
                RefTrackerContextHandle rtctxhandle,
                ModuleSet& module_set,
                Uptr<TaskManager> task_mgr,
                Uptr<JobManager> job_mgr);
private:
  CustomApi* custom_api_{ nullptr };
  io_context& ioc_;
  int session_id_;
  Uptr<TaskManager> task_mgr_;
  Uptr<JobManager> job_mgr_; // aggregates its own taskmgr
  Uptr<CmdExecutor> cmd_executor_;
  ModuleDataSlotVector module_slots_;
};

}
