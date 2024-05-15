#pragma once

#include "zca/netshell_status_descriptor_table.h"
#include "netshell/ns_cmd_result.h"

namespace core {

static netshell::NsCmdResult ResWithMsgBody(int retcode, const std::string& msg) {
  return netshell::NsCmdResult(kNsCmdExecuted, retcode, netshell::NsResultType::kMessage)
    .WithMessageBody(msg);
}

}

