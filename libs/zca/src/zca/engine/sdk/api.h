#pragma once

#include "co/common.h"

namespace engine {

class Module;
class ModuleEngine;

// Module's api
class ModuleApi {
public:
  virtual ~ModuleApi() = default;
};

// Engine's local api
class LocalApi {
public:
  virtual ~LocalApi() = default;

protected:
  Module& GetModule() {
    DCHECK(client_module_ != nullptr);
    return *client_module_;
  }

private:
  friend class ModuleEngine;
  void ModuleConnect(Module& client_module) {
    client_module_ = &client_module;
  }
private:
  Module* client_module_;
};

// Engine's global api
class GlobalApi {
public:
  virtual ~GlobalApi() = default;
};


}

