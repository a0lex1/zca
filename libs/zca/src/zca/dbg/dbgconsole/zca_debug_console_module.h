#pragma once

// TODO: remove old code everywhere.

#include "co/dbg/dbgconsole/debug_console_module.h"

namespace zca { // only for zca::dbg
namespace dbg {
namespace dbgconsole {

class ZcaDebugConsoleModule: public co::dbg::dbgconsole::DebugConsoleModule {
public:
  virtual ~ZcaDebugConsoleModule() = default;

  const co::dbg::dbgconsole::DispatchMap& GetDispatchMap() override;

private:
  void cmd_stats(const std::string&, std::string&);
};

}}}
