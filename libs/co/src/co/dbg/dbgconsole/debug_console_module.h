#pragma once

#include "co/dbg/dbgconsole/dispatch_map.h"

namespace co {
namespace dbg {
namespace dbgconsole {

namespace detail {
class DebugConsoleServerSession;
}

class DebugConsoleModule {
public:
  virtual ~DebugConsoleModule() = default;

private:
  friend class detail::DebugConsoleServerSession;

  virtual const DispatchMap& GetDispatchMap() = 0;
};

}}}
