#pragma once

#include "zca/engine/sdk/module.h"

#include "netshell/ns_para_command_executor.h"

namespace core {
namespace front {

class FrontendCore;
class FrontendLocalApi;
class FrontendGlobalApi;
class FrontendModuleApi;

// To implement by module, frontend uses it
class FrontendModuleApi : public engine::ModuleApi {
public:
  virtual ~FrontendModuleApi() = default;

};

// Local for each module - side of frontend API
class FrontendLocalApi : public engine::LocalApi {
public:
  virtual ~FrontendLocalApi() = default;


private:
  friend class FrontendCore;
  FrontendLocalApi() {}
};


// Global for all modules - side of frontend API
class FrontendGlobalApi : public engine::GlobalApi {
public:
  virtual ~FrontendGlobalApi() = default;

  using NsParaCommandExecutor = netshell::NsParaCommandExecutor;

  FrontendGlobalApi()
  {
  }
private:

};

}}
