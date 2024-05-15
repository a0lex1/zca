#include "zca/engine/sdk/dispatch.h"
#include "zca/engine/sdk/job_manager.h"
#include "zca/engine/sdk/cmd_executor.h"
#include "zca/engine/sdk/engine_session_core.h"

using namespace std;

namespace engine {

DispatchContext::DispatchContext(TaskManager& task_mgr,
                                 CmdExecutor& cmd_executor,
                                 EngineSessionCore& session_core)
  :
  task_mgr_(task_mgr), cmd_executor_(cmd_executor), session_core_(session_core)
{
}

DispatchContext::TaskManager& DispatchContext::GetTaskManager() {
  return task_mgr_;
}

CmdExecutor& DispatchContext::GetCmdExecutor() {
  return cmd_executor_;
}

EngineSessionApi& DispatchContext::GetSessionApi() {
  return session_core_.GetSessionApi();
}

const netshell::NsCmdResult* DispatchCmdData::GetInputNsResult() const {
  return input_ns_result_;
}

const string& DispatchCmdData::GetNsCommand() const {
  return raw_cmd_;
}

string& DispatchCmdData::GetNsCommand() {
  return const_cast<string&>(
    const_cast<const DispatchCmdData*>(this)->GetNsCommand());
}

const netshell::NsCmdResult& DispatchCmdData::GetNsResult() const {
  return *ns_result_;
}

netshell::NsCmdResult& DispatchCmdData::GetNsResult() {
  return const_cast<netshell::NsCmdResult&>(
    const_cast<const DispatchCmdData*>(this)->GetNsResult());
}

SessionModuleData* DispatchCmdData::GetSessionModuleData() {
  return sess_module_data_;
}

// -----------------------------------------------------------------------------------------------------------------------------------------------

bool DispatchCmdData::Empty() const {
  return raw_cmd_.empty() && /*ns_result_.empty() &&*/ sess_module_data_ == nullptr;
}

// -----------------------------------------------------------------------------------------------------------------------------------------------

void DispatchTable::AddCommand(const string& prog_name, DispatchCmdFunc disp_func) {
  auto it = cmd_map_.find(prog_name);
  DCHECK(it == cmd_map_.end());
  cmd_map_[prog_name] = disp_func;
}

const DispatchCmdFuncMap& DispatchTable::GetMap() const {
  return cmd_map_;
}

DispatchCmdFuncMap& DispatchTable::GetMap() {
  return const_cast<DispatchCmdFuncMap&>(const_cast<const DispatchTable*>(this)->GetMap());
}


}
