#pragma once

#include "cc/cc_client_command_dispatcher.h"

namespace cc {

class CcClientCmdDispFromFunc : public CcClientCommandDispatcher {
public:
  virtual ~CcClientCmdDispFromFunc() = default;

  using CmdHandlerFunc = Func<void(Uptr<std::string>, std::string&, EmptyHandler)>;

  void SetCommandHandler(CmdHandlerFunc cmd_handler) { user_handler_ = cmd_handler; }

  // CcClientCommandDispatcher impl.
  void DispatchCommand(Uptr<std::string> cmd_opaque_data,
                       std::string& cmd_result_opaque_data,
                       EmptyHandler handler) override {
    if (user_handler_) {
      user_handler_(std::move(cmd_opaque_data), cmd_result_opaque_data, handler);
    }
  }
private:
  CmdHandlerFunc user_handler_;
}; 

}


