#include "zca/engine/sdk/dispatch_cmd_task.h"

namespace engine {

DispatchCmdTask::DispatchCmdTask(Shptr<Strand> strand)
  :
  TaskImpl(strand),
  disp_context_(nullptr), disp_data_(nullptr)
{

}

void DispatchCmdTask::SetResult(const netshell::NsCmdResult& result) {
  if (!bkg_) {
    disp_data_->GetNsResult() = result;
  }
  else {
    // We were converted to background task. Do nothing. Reserved for future. Can be something like printing
    // the result to the log.
  }
}

Module& DispatchCmdTask::GetModule() {
  DCHECK(module_ != nullptr);
  return *module_;
}

engine::DispatchContext& DispatchCmdTask::GetDispatchContext() {
  return *disp_context_.get();
}

void DispatchCmdTask::SetModule(Module& module) {
  DCHECK(!module_);
  module_ = &module;
}



}

