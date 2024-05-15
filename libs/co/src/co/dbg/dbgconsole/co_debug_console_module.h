#pragma once

#include "co/dbg/dbgconsole/debug_console_module.h"

namespace co {
namespace dbg {
namespace dbgconsole {

class CoDebugConsoleModule: public co::dbg::dbgconsole::DebugConsoleModule {
public:
  virtual ~CoDebugConsoleModule() = default;

  const co::dbg::dbgconsole::DispatchMap& GetDispatchMap() override;

private:
  void cmd_rts(const std::string&, std::string&);
};

}}}

