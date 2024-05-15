#pragma once

#include "co/common.h"

namespace engine {

class Module;
class LocalApi;
class GlobalApi;

class ModuleSet {
public:
  virtual ~ModuleSet() = default;

  virtual void AddModule(Uptr<Module> module, Uptr<LocalApi> local_api) = 0;
  virtual size_t GetModuleCount() const = 0;
  virtual Module& GetModule(size_t module_idx) = 0;
};

}

