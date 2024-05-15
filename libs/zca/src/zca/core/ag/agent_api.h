#pragma once

#include "zca/engine/sdk/module.h"

#include "cc/cc_client.h"

namespace core {
namespace ag {

  // See comments in backend_api.h

class AgentCore;

class AgentModuleApi : public engine::ModuleApi {
public:
  virtual ~AgentModuleApi() = default;
};

class AgentLocalApi : public engine::LocalApi {
public:
  virtual ~AgentLocalApi() = default;

private:
  friend class AgentCore;
  AgentLocalApi() {}
};  

class AgentGlobalApi : public engine::GlobalApi {
public:
  virtual ~AgentGlobalApi() = default;
    
  cc::CcClient& GetCcClient() { return cc_client_; }

private:
  friend class AgentCore;
  AgentGlobalApi(cc::CcClient& cc_client) : cc_client_(cc_client) {}
  
private:
  cc::CcClient& cc_client_;
};  
  
}}






