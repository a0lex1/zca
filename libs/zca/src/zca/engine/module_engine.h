#pragma once

// Entry point include file

#include "zca/engine/sdk/module.h"

#include "co/async/threadsafe_stopable_impl.h"

#include "co/base/debug_tag_owner.h"

namespace engine {

class ModuleEngine
  :
  public ModuleSet,
  public co::DebugTagOwner
{
public:
  virtual ~ModuleEngine();

  using RefTrackerContext = co::RefTrackerContext;
  using RefTrackerContextHandle = co::RefTrackerContextHandle;
  using RefTracker = co::RefTracker;
  using TaskManager = co::async::TaskManager;

  ModuleEngine(GlobalApi& global_api);

  // [ModuleSet impl]
  void AddModule(Uptr<Module> module, Uptr<LocalApi> local_api) override;
  size_t GetModuleCount() const override;
  Module& GetModule(size_t module_idx) override;

  //void DoInitialization(); // example

  // Tasks are attached to |rtctxhandle|
  // Details: Each engine's user session aggregates EngineSession which
  // interacts with ModuleEngine
  Uptr<EngineSession> CreateEngineSession(Shptr<Strand> strand,
                                          RefTrackerContextHandle rtctxhandle,
                                          io_context& ioc,
                                          Uptr<TaskManager> task_mgr,
                                          Uptr<JobManager> job_mgr);
private:

private:
  GlobalApi& global_api_;
  int cur_sess_id_{ 0 };
  std::vector<Uptr<Module>> modules_;
};

}

