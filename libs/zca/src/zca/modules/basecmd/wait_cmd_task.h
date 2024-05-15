#pragma once

#include "zca/engine/sdk/dispatch_cmd_task.h"
#include "zca/core/res_with_msg_body.h"

#include "co/base/cmdline/parse_cmdline_to_argv.h"

#include <sstream>

namespace modules {
namespace basecmd {

class WaitCmdTask
  :
  public engine::DispatchCmdTask,
  public co::enable_shared_from_this<WaitCmdTask>
{
public:
  virtual ~WaitCmdTask() = default;

  WaitCmdTask(Shptr<Strand> strand)
    :
    DispatchCmdTask(strand)
  {
  }

private:
  void BeginIo(co::RefTracker rt) override {
    rt.SetReferencedObject(shared_from_this());

    timer_ = make_unique<boost::asio::deadline_timer>(
      GetDispatchContext().GetSessionApi().GetSessionIoContext());

    auto& cmd = GetNsCommand();
    StringVector argv;
    if (!co::cmdline::ParseCmdlineToArgv(cmd.c_str(), argv)) {
      SetResult(core::ResWithMsgBody(-1, "can't parse cmdline"));
      return;
    }
    if (argv.size() != 2) {
      SetResult(core::ResWithMsgBody(-2, "bad arg count"));
      return;
    }
    uint32_t timeout_msec;
    DCHECK(argv[0] == "wait");
    if (!co::string_to_uint(argv[1], timeout_msec)) {
      SetResult(core::ResWithMsgBody(-3, "bad arg1 integer"));
      return;
    }
    timer_->expires_from_now(boost::posix_time::milliseconds(timeout_msec));
    timer_->async_wait(co::bind(&WaitCmdTask::HandleWait, shared_from_this(),
                       _1, rt));
  }
  void StopUnsafeExtra() override {
    timer_->cancel();
  }
  void HandleWait(Errcode err, RefTracker rt) {
    if (err) {
      std::stringstream ss;
      ss << err;
      SetResult(core::ResWithMsgBody(-4, std::string("errcode ") + ss.str()));
    }
    else {
      SetResult(core::ResWithMsgBody(0, "wait succeeded"));
    }
    // rt will be dereferenced
  }
private:
  Uptr<boost::asio::deadline_timer> timer_;
};


}}
