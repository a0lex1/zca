#pragma once

#include "co/async/session.h"
#include "co/net/endpoint.h"

namespace engine {

class EngineSessionInfo { // for all
public:
  virtual ~EngineSessionInfo() = default;

  virtual int GetSessionId() const = 0;
  //virtual co::net::Endpoint GetRemoteAddress() = 0;
};

class CmdExecutor;
class EngineSessionCore;
class EngineSessionApi { // for modules
public:
  virtual ~EngineSessionApi() = default;

  virtual const EngineSessionInfo& GetSessionInfo() = 0;
  virtual io_context& GetSessionIoContext() = 0;
  virtual void KillSession() = 0; // threadsafe

private:
  friend class CmdExecutor;
  virtual EngineSessionCore& GetSessionCore() = 0;
};

class SessionModuleData {
public:
  virtual ~SessionModuleData() = default;
  // Each module can hold any data in each session, derive from this class
};

// -------------------------------
// Internal

// rename : SessionCore -> ContextCore -> ContextCore
class EngineSessionCore {
public:
  virtual ~EngineSessionCore() = default;

  virtual EngineSessionApi& GetSessionApi() = 0;
  virtual SessionModuleData* GetSessionModuleDataFor(size_t module_idx) = 0;
//  virtual io_context& GetIoContext() = 0;
};

}
