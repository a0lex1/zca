#pragma once

#include "co/common.h"

namespace cc {

class CcClientCmdTask {
public:
  virtual ~CcClientCmdTask() = default;
};

class CcClientCommandDispatcher {
public:
  virtual ~CcClientCommandDispatcher() = default;

  virtual void DispatchCommand(Uptr<std::string> cmd_opaque_data,
                               std::string& cmd_result_opaque_data,
                               EmptyHandler handler) = 0;
};

}
