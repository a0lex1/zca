#include "zca/netshell_status_descriptor_table.h"

using namespace netshell;

// declared 'extern' in .h
NsStatusDescriptorTable gZcaNsStatusDescriptorTable = {
  {kNsCmdNoResult, {"NO_RESULT", fCanNotHaveBody}},
  {kNsCmdExecuted, {"CMD_OK", fCanHaveBody}},
  {kNsCmdNotFound, {"CMD_NOT_FOUND", fCanNotHaveBody}},
  {kNsBackNotConnected, {"BACK_NOT_CONNECTED", fCanNotHaveBody}},
};

