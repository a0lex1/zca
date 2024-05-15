#pragma once

#include "zca/engine/sdk/dispatch.h"

#include "netshell/ns_cmd_result.h"

#include "co/async/task_manager.h"

namespace engine {

class ModuleSet;
class EngineSessionApi;
class CustomApi;

class CmdExecutor {
public:
  using NsCmdResultVector = netshell::NsCmdResultVector;
  using RefTrackerContext = co::RefTrackerContext;
  using RefTrackerContextHandle = co::RefTrackerContextHandle;
  using TaskManager = co::async::TaskManager;
  using Task = co::async::Task;

  // |rtctxhandle| is used to attach task(s) to
  CmdExecutor(Shptr<Strand> strand,
              RefTrackerContextHandle rtctxhandle,
              ModuleSet& module_set,
              TaskManager& task_mgr,
              int ns_code_for_cmd_not_found);

  // Calling code should wrap its |handler| to strand if needed, because it will posted
  // without any strand wrapping.
  void ExecuteCommand(
    CustomApi* custom_api,
    EngineSessionApi& sess_api,
    const netshell::NsCmdResult* input_ns_result,
    const std::string& raw_cmd,
    netshell::NsCmdResult& ns_result,
    EmptyHandler handler,
    Shptr<Task>& task_spawned);

  ModuleSet& GetModuleSet();

  TaskManager& GetTaskManager();

private:
  bool FindDispatchFunc(
    const std::string& program_name,
    DispatchCmdFunc& func_found,
    size_t& module_index);

private:
  Shptr<Strand> strand_;
  RefTrackerContextHandle rtctxhandle_;
  ModuleSet& module_set_;
  TaskManager& task_mgr_;
  int ns_code_for_cmd_not_found_;
};

}
