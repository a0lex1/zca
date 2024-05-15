#include "zca/dbg/dbgconsole/zca_debug_console_module.h"

using namespace std;
using namespace co::dbg::dbgconsole;

namespace zca { // only for zca::dbg
namespace dbg {
namespace dbgconsole {


const DispatchMap& ZcaDebugConsoleModule::GetDispatchMap() {
  static const DispatchMap& disp_map = {
    { "stats", co::bind(&ZcaDebugConsoleModule::cmd_stats, this, _1, _2) },
  };
  return disp_map;
}

void ZcaDebugConsoleModule::cmd_stats(const string& inp, string& outp) {
  outp = "TODO!";

}


}}}
