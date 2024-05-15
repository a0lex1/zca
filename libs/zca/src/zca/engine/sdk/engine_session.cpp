#include "zca/engine/sdk/job_manager.h"
#include "zca/engine/sdk/engine_session.h"
#include "zca/engine/sdk/module.h"

#include "zca/netshell_status_descriptor_table.h"

using namespace std;
using namespace co;

namespace engine {

EngineSession::EngineSession(Shptr<Strand> strand,
                             io_context& ioc,
                             int session_id,
                             RefTrackerContextHandle rtctxhandle,
                             ModuleSet& module_set,
                             Uptr<TaskManager> task_mgr,
                             Uptr<JobManager> job_mgr)
  :
  Fibered(strand),
  ioc_(ioc),
  session_id_(session_id),
  task_mgr_(move(task_mgr)),
  job_mgr_(move(job_mgr)),
  cmd_executor_(make_unique<CmdExecutor>(strand, rtctxhandle, module_set, *task_mgr_.get(),
    kNsCmdNotFound))
{

}

void EngineSession::InitializeSlots()
{
  // Create data slots for modules inside this session
  auto& module_set(cmd_executor_->GetModuleSet());
  auto module_count(cmd_executor_->GetModuleSet().GetModuleCount());
  for (size_t i = 0;
       i < module_count;
       i++)
  {
    auto& admin_module = static_cast<Module&>(module_set.GetModule(i));
    Uptr<SessionModuleData> module_data;
    admin_module.OnSessionCreated(*this, module_data);
    // |module_data| can be nullptr if module doesn't want to keep it's opaque session data
    module_slots_.push_back(std::move(module_data));
    DCHECK(admin_module.GetModuleIndex() == module_slots_.size() - 1);
  }
}

void EngineSession::Start(co::RefTracker rt)
{
  task_mgr_->Start(co::RefTracker(CUR_LOC(), [&]() {
                   },
                   rt));
}

void EngineSession::StopThreadsafe()
{
  // inside unknown fiber

  task_mgr_->StopThreadsafe();
}

void EngineSession::ExecuteCommand(const netshell::NsCmdResult* input_ns_result,
                                   const std::string& raw_cmd,
                                   netshell::NsCmdResult& ns_result,
                                   EmptyHandler handler,
                                   Shptr<Task> task_spawned)
{
  cmd_executor_->ExecuteCommand(custom_api_, *this, input_ns_result, raw_cmd,
    ns_result, handler, task_spawned);
}

EngineSessionApi& EngineSession::GetSessionApi()
{
  return *this;
}

SessionModuleData* EngineSession::GetSessionModuleDataFor(size_t module_idx)
{
  DCHECK(module_idx < module_slots_.size());
  return module_slots_[module_idx].get();
}

const EngineSessionInfo& EngineSession::GetSessionInfo()
{
  return *this;
}

io_context& EngineSession::GetSessionIoContext()
{
  return ioc_;
}

void EngineSession::KillSession()
{
  StopThreadsafe();
}

EngineSessionCore& EngineSession::GetSessionCore()
{
  return *this;
}

int EngineSession::GetSessionId() const
{
  return session_id_;
}

}


