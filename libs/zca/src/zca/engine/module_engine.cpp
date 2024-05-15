#include "zca/engine/module_engine.h"

#include "co/xlog/xlog.h"

using namespace std;

namespace engine {

#define llog() syslog(_DBG) << "ModEng " << SPTR(this) << " `" << GET_DEBUG_TAG(*this) << "` "

ModuleEngine::~ModuleEngine() {
  llog() << "~~~DTOR~~~\n";
}

ModuleEngine::ModuleEngine(GlobalApi& global_api)
  :
  global_api_(global_api)
{
  llog() << "CTOR\n";
}

Uptr<engine::EngineSession> ModuleEngine::CreateEngineSession(
  Shptr<Strand> strand,
  RefTrackerContextHandle rtctxhandle,
  io_context& ioc,
  Uptr<TaskManager> task_mgr,
  Uptr<JobManager> job_mgr)
{
  return Uptr<EngineSession>(new EngineSession(
    strand,
    ioc,
    cur_sess_id_++,
    rtctxhandle,
    *this, // ModuleSet&
    move(task_mgr),
    move(job_mgr)));
}

void ModuleEngine::AddModule(Uptr<Module> module, Uptr<LocalApi> local_api)
{
  size_t module_index = modules_.size(); // next index

  // beeeeep beep
  local_api->ModuleConnect(*module.get());

  // beeeeeeeeeep
  module->EngineConnect(module_index, std::move(local_api), global_api_);

  // beep
  modules_.emplace_back(std::move(module));
}

size_t ModuleEngine::GetModuleCount() const
{
  return modules_.size();
}

engine::Module& ModuleEngine::GetModule(size_t module_idx)
{
  DCHECK(module_idx < modules_.size());
  return *modules_[module_idx].get();
}


}


