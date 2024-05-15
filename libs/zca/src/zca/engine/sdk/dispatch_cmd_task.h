#pragma once

// Entry point header file

#include "zca/engine/sdk/module.h"

namespace engine {

class DispatchTaskSharedData {
public:
  // For jobs FUTURE
};

class CustomApi;
class DispatchCmdTask : public co::async::TaskImpl {
public:
  virtual ~DispatchCmdTask() = default;

  DispatchCmdTask(Shptr<Strand> strand);

protected:
  // Can't be called until the task is Start()ed
  DispatchContext& GetDispatchContext();
  //DispatchCmdData& GetDispatchData(); // we hide it, allowing only some functionality to be open for derived class(es):
  const std::string& GetNsCommand() const { return disp_data_->GetNsCommand(); }
  void SetResult(const netshell::NsCmdResult& result);
  SessionModuleData* GetSessionModuleData() { return disp_data_->GetSessionModuleData();}
  template <typename T> T* GetSessionModuleDataAs() {
    return static_cast<T*>(this->GetSessionModuleData());
  }
  CustomApi* GetCustomApi() { return custom_api_; }
  template <typename T> T* GetCustomApiAs() { return static_cast<T*>(GetCustomApi()); }

  Module& GetModule();

  // From BeginIo you can start using GetDispatchContext(), etc.
  // 
  // Implement these:
  // Start(rt)
  // StopThreadsafe()

private:
  friend class CmdExecutor;
  void SetModule(Module& module);

private:
  bool bkg_{ false };
  Module* module_{ nullptr };
  Uptr<DispatchContext> disp_context_;
  Uptr<DispatchCmdData> disp_data_;
  CustomApi* custom_api_{ nullptr };
};

}

