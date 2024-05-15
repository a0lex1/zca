#pragma once

#include "netshell/ns_cmd_result.h"

#include "co/async/task_manager.h"

namespace engine {

class JobManager;
class CmdExecutor;
class CmdListExecutionTask;
class EngineSessionCore;
class EngineSessionApi;
class SessionModuleData;
class DispatchCmdTask;

class DispatchContext {
public:
  virtual ~DispatchContext() = default;

  using TaskManager = co::async::TaskManager;

  DispatchContext(TaskManager& task_mgr, CmdExecutor& cmd_executor,
                  EngineSessionCore& session_core);

  JobManager& GetJobManager();
  CmdExecutor& GetCmdExecutor();
  EngineSessionApi& GetSessionApi();

  // Default
  TaskManager& GetTaskManager();

private:
  TaskManager& task_mgr_;
  CmdExecutor& cmd_executor_;
  EngineSessionCore& session_core_;

  friend class CmdExecutor;
};

class DispatchCmdData {
public:
  virtual ~DispatchCmdData() = default;

  const netshell::NsCmdResult* GetInputNsResult() const;
  const std::string& GetNsCommand() const;
  const netshell::NsCmdResult& GetNsResult() const;

  SessionModuleData* GetSessionModuleData();

  template <typename T>
  T* GetSessionModuleDataAs() { return static_cast<T*>(sess_module_data_); }

  // const-nonconst
  std::string& GetNsCommand();
  netshell::NsCmdResult& GetNsResult();

private:
  // [DispatchCmdData impl]
  bool Empty() const;

  friend class CmdExecutor;

  const netshell::NsCmdResult* input_ns_result_;
  std::string raw_cmd_;
  netshell::NsCmdResult* ns_result_;
  SessionModuleData* sess_module_data_;
};


// --------------------------------------------------------
// Dispatch table func prototypes
// --------------------------------------------------------
using DispatchCmdFunc
  = Func<void(DispatchContext&, DispatchCmdData&, Shptr<Strand>, Shptr<DispatchCmdTask>& new_task)>;

using DispatchCmdFuncMap
  = std::map<std::string, DispatchCmdFunc>;


// --------------------------------------------------------
// Dispatch table
// --------------------------------------------------------
class DispatchTable {
public:
  DispatchTable() {}

  //void SetMap(const DispatchCmdFuncMap& disp_cmd_map);
  void AddCommand(const std::string& prog_name, DispatchCmdFunc disp_func);

  const DispatchCmdFuncMap& GetMap() const;
  DispatchCmdFuncMap& GetMap();

private:
  friend class CmdListExecutionTask;
  DispatchCmdFuncMap cmd_map_;
};


}
