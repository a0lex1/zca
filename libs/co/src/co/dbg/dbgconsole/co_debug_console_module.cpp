#include "co/dbg/dbgconsole/co_debug_console_module.h"

#include "co/base/ref_tracker.h"

using namespace std;
using namespace co::dbg::dbgconsole;

namespace co {
namespace dbg {
namespace dbgconsole {


const DispatchMap& CoDebugConsoleModule::GetDispatchMap() {
  static const DispatchMap& disp_map = {
    { "rts", co::bind(&CoDebugConsoleModule::cmd_rts, this, _1, _2) },
  };
  return disp_map;
}

void CoDebugConsoleModule::cmd_rts(const string& inp, string& outp) {
  stringstream ss;
  co::PrintRefTrackerContextDataList(ss);
  outp = ss.str();
}


}}}
