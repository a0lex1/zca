#include "zca/engine/sdk/module.h"

namespace engine {

engine::LocalApi& ModuleBase::GetLocalApi()
{
  return *local_api_.get();
}

engine::GlobalApi& ModuleBase::GetGlobalApi()
{
  return *global_api_;
}

void ModuleBase::EngineConnect(size_t module_index,
                               Uptr<LocalApi> loc_api,
                               GlobalApi& glob_api)
{
  DCHECK(module_index_ == -1); // ensure not yet connected
  DCHECK(module_index != -1);
  module_index_ = module_index;
  local_api_ = std::move(loc_api);
  global_api_ = &glob_api;
}

size_t ModuleBase::GetModuleIndex() const
{
  return module_index_;
}


}
