#pragma once

// Entry point header file

#include "zca/engine/sdk/engine_session.h"
#include "zca/engine/sdk/api.h"
#include "zca/engine/sdk/job_manager.h"
#include "zca/engine/sdk/cmd_executor.h"
#include "zca/engine/sdk/dispatch.h"
#include "zca/engine/sdk/module_set.h"

#include "co/async/wrap_post.h"
#include "co/async/task_manager.h"

#include "co/base/debug_tag_owner.h"

namespace engine {

// Helpful to separate ModuleBase and Module
class ModuleBase {
public:
  virtual ~ModuleBase() = default;

  size_t GetModuleIndex() const;
  
  virtual ModuleApi& GetModuleApi() = 0;
  template <typename T>
  T& GetModuleApiAs() { return static_cast<T&>(GetModuleApi()); }

  LocalApi& GetLocalApi();
  GlobalApi& GetGlobalApi();

  template <typename T>
  T& GetLocalApiAs() { return static_cast<T&>(GetLocalApi()); }

  template <typename T>
  T& GetGlobalApiAs() { return static_cast<T&>(GetGlobalApi()); }
    
  bool AddedToEngine() const { return module_index_ != (size_t)-1; }

private:
  // Called by ModuleEngine when it adds this module
  friend class ModuleEngine;
  void EngineConnect(size_t module_index, Uptr<LocalApi> loc_api, GlobalApi& glob_api);

private:
  size_t module_index_{ (size_t)-1 };
  Uptr<LocalApi> local_api_;
  GlobalApi* global_api_;
};

/*
* class Module
* 
*/

class Module : public ModuleBase, public co::DebugTagOwner
{
public:
  virtual ~Module() = default;

  using RefTracker = co::RefTracker;
private:
  virtual const DispatchCmdFuncMap& GetDispatchCmdFuncMap() = 0;
  
  virtual void OnSessionCreated(EngineSessionApi& sess_api,
                                Uptr<SessionModuleData>& module_data) = 0;

  friend class EngineSession;
  friend class CmdExecutor;
  friend class AdminSession;
};

}
