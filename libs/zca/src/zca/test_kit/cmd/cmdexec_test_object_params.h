#pragma once

#include "zca/netshell_status_descriptor_table.h"

#include "netshell/ns_cmd_result.h"

#include "cc/bot_id.h"
#include "cc/cc_error.h"

#include <boost/date_time/posix_time/ptime.hpp>

using CmdexecTestResultCheckFn = Func<void(const netshell::NsCmdResult&)>;
using CmdexecTestCcErrstackCheckFn = Func<void(const std::vector<cc::CcError>&)>;

struct CmdexecTestObjectParams {

  using time_duration = boost::posix_time::time_duration;

  CmdexecTestObjectParams(
    const cc::BotId& _bot_id,
    const std::string& _cmd,
    CmdexecTestResultCheckFn _resultchk_fn_,
    CmdexecTestCcErrstackCheckFn _ccerrstackchk_fn,
    const time_duration& _delay_before_stop = time_duration())
    :
    bot_id(_bot_id), cmd(_cmd),
    resultchk_fn(_resultchk_fn_),
    ccerrstackchk_fn(_ccerrstackchk_fn),
    delay_before_stop(_delay_before_stop)
  {

  }

  cc::BotId bot_id;
  std::string cmd;
  time_duration delay_before_stop = time_duration();
  CmdexecTestResultCheckFn resultchk_fn;
  CmdexecTestCcErrstackCheckFn ccerrstackchk_fn;
};

// resultchk_fn_  for checking the |r| is `command scheduled` result
static void checkerForCmdScheduled(const netshell::NsCmdResult& r) {
  DCHECK(r.status_code == kNsCmdExecuted);
  DCHECK(r.ret_code == 0);
  DCHECK(r.result_type == netshell::NsResultType::kMessage);
  DCHECK(r.body_line_count == 0);
}

static void checkerForCcErrstackSuccess(const std::vector<cc::CcError>& cc_err_stack) {
  DCHECK(cc_err_stack.size() == 0);
}

static void EmptyCheckerForCcErrstack(const std::vector<cc::CcError>& cc_err_stack) {
}