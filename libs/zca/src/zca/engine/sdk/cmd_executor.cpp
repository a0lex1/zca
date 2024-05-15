#include "zca/engine/sdk/cmd_executor.h"
#include "zca/engine/sdk/dispatch.h"
#include "zca/engine/sdk/dispatch_cmd_task.h"
#include "zca/engine/sdk/module.h"

#include "co/base/strings.h"
#include "co/base/cmdline/intermed_cmd_repr.h"
#include "co/xlog/xlog.h"

#include <boost/asio/post.hpp>

using namespace std;
using namespace netshell;
using namespace co;
using co::cmdline::IntermedCmdRepr;

namespace engine {

CmdExecutor::CmdExecutor(
  Shptr<Strand> strand,
  RefTrackerContextHandle rtctxhandle,
  ModuleSet& module_set,
  TaskManager& task_mgr,
  int ns_code_for_cmd_not_found)
  :
  strand_(strand),
  rtctxhandle_(rtctxhandle), module_set_(module_set), task_mgr_(task_mgr),
  ns_code_for_cmd_not_found_(ns_code_for_cmd_not_found)
{
  // We save a |rtctxhandle_|. We attach tasks to it.
}

// |ns_result| is owned by a caller until |handler| is executed
void CmdExecutor::ExecuteCommand(
    CustomApi* custom_api,
    EngineSessionApi& sess_api,
    const netshell::NsCmdResult* input_ns_result,
    const std::string& raw_cmd,
    netshell::NsCmdResult& ns_result,
    EmptyHandler handler,
    Shptr<Task>& task_spawned) {

  IntermedCmdRepr cmd_repr(raw_cmd);
  auto prog_name(make_shared<string>(cmd_repr.parts[0])); // repr must succeed

  DispatchCmdFunc disp_func;
  size_t module_idx;

  if (FindDispatchFunc(*prog_name.get(), disp_func, module_idx)) {

    Uptr<DispatchContext> disp_ctx = make_unique<DispatchContext>(
      task_mgr_, *this, sess_api.GetSessionCore()
    );
    Uptr<DispatchCmdData> disp_data = make_unique<DispatchCmdData>();
    disp_data->input_ns_result_ = input_ns_result;
    disp_data->raw_cmd_ = raw_cmd;
    disp_data->ns_result_ = &ns_result;
    disp_data->sess_module_data_ = disp_ctx->session_core_.GetSessionModuleDataFor(module_idx);

    Shptr<DispatchCmdTask> new_task;

    //
    // Call module dispatch func for the command
    //
    disp_func(*disp_ctx.get(), *disp_data.get(), strand_, new_task);

    if (new_task == nullptr) {

      // Immediate complete.
      // Don't clear the |handler|. The result is in |ns_result|
    }
    else {
      // Schedule task that user has returned
      // Prepare and execute the task
      // Every task is connected to module that issued it
      new_task->SetModule(module_set_.GetModule(module_idx));

      new_task->disp_context_ = std::move(disp_ctx);
      new_task->disp_data_ = std::move(disp_data);
      new_task->custom_api_ = custom_api;

      //
      // Execute user's task on TaskManager
      // Executing without any strand. Calling code should wrap its |handler| to strand if needed
      // 
      //
      task_mgr_.ExecuteTask(
        new_task,
        RefTracker(CUR_LOC(), rtctxhandle_, handler));

      // A task that we've just executed is now carrying |handler|
      // Don't post it, we've just executed a new task, it carries the handler now
      handler = nullptr;
    }
  }
  else {
    //
    // Command not found
    //
    ns_result = NsCmdResult(ns_code_for_cmd_not_found_);
  }

  // Post if not cleared
  if (handler != nullptr) {
    // Can be recursive call, post it
    // Posting without any strand. Calling code should wrap its |handler| to strand if needed
    boost::asio::post(sess_api.GetSessionCore().GetSessionApi().GetSessionIoContext(),
      handler);
  }
}

bool CmdExecutor::FindDispatchFunc(const string& program_name,
  DispatchCmdFunc& func_found,
  size_t& module_index)
{
  for (size_t i = 0; i < module_set_.GetModuleCount(); i++) {
    Module& module(static_cast<Module&>(module_set_.GetModule(i)));

    const auto& disp_map = module.GetDispatchCmdFuncMap();

    for (const auto& it : disp_map) {
      if (it.first == program_name) {
        func_found = it.second;
        module_index = i;
        return true;
      }
    }
  }
  return false;
}

engine::ModuleSet& CmdExecutor::GetModuleSet() {
  return module_set_;
}

engine::CmdExecutor::TaskManager& CmdExecutor::GetTaskManager() {
  return task_mgr_;
}

}
