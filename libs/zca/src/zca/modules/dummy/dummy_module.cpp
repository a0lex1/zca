#include "zca/modules/dummy/dummy_module.h"

#include "zca/engine/sdk/dispatch_cmd_task.h"

#include "zca/netshell_status_descriptor_table.h"

#include "co/base/cmdline/intermed_cmd_repr.h"
#include "co/base/cmdline/parse_cmdline_to_argv.h"
#include "co/base/strings.h"

#include <boost/asio/deadline_timer.hpp>

#include <sstream>

using namespace std;
using namespace co;
using namespace co::cmdline;
using namespace netshell;
using namespace engine;
using co::cmdline::IntermedCmdRepr;

namespace modules {
namespace dummy {

namespace {
class DummySessionModuleData : public SessionModuleData {
public:
  virtual ~DummySessionModuleData() = default;

  DummySessionModuleData() : num_cmds_executed_(0) {}

  size_t num_cmds_executed_;
  string session_tag_;
};

class WaitCmdTask
  :
  public DispatchCmdTask,
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
    if (!ParseCmdlineToArgv(cmd.c_str(), argv)) {
      SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
        .WithMessageBody("can't parse cmdline"));
      return;
    }
    if (argv.size() != 2) {
      SetResult(NsCmdResult(kNsCmdExecuted, -2, NsResultType::kMessage)
        .WithMessageBody("bad arg count"));
      return;
    }
    uint32_t timeout_msec;
    DCHECK(argv[0] == "wait");
    if (!co::string_to_uint(argv[1], timeout_msec)) {
      SetResult(NsCmdResult(kNsCmdExecuted, -3, NsResultType::kMessage)
        .WithMessageBody("bad arg1 integer"));
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
      stringstream ss;
      ss << err;
      SetResult(NsCmdResult(kNsCmdExecuted, -4, NsResultType::kMessage)
        .WithMessageBody(string("errcode ") + ss.str()));
    }
    else {
      SetResult(NsCmdResult(kNsCmdExecuted, 0, NsResultType::kMessage)
        .WithMessageBody("wait succeeded"));
    }
    auto our_data = static_cast<DummySessionModuleData*>(GetSessionModuleData());
    our_data->num_cmds_executed_ += 1;

    // rt will be dereferenced
  }
private:
  Uptr<boost::asio::deadline_timer> timer_;
};


class ExecCmdTask
  :
  public DispatchCmdTask, public co::enable_shared_from_this<ExecCmdTask>
{
public:
  virtual ~ExecCmdTask() = default;

  ExecCmdTask(Shptr<Strand> strand)
    : DispatchCmdTask(strand)
  {
  }

private:
  void BeginIo(RefTracker rt) override {
    auto& cmd = GetNsCommand();
    IntermedCmdRepr repr(cmd);
    DCHECK(repr.parts[0] == "exec");
    DCHECK(repr.parts.size() > 1);
    repr.parts.erase(repr.parts.begin());
    string subj_cmd(repr.Join());

    GetDispatchContext().GetCmdExecutor().ExecuteCommand(
      GetCustomApi(),
      GetDispatchContext().GetSessionApi(),
      nullptr,
      subj_cmd,
      ns_result_,
      co::bind(&ExecCmdTask::HandleExecuteCommandList, shared_from_this(), rt),
      subtask_);
  }
  void StopUnsafeExtra() override {
    if (subtask_) {
      //subtask_->StopUnsafe();
      subtask_->StopThreadsafe();
    }
  }
  void HandleExecuteCommandList(RefTracker rt) {
    // todo: fill unfilled
    subtask_ = nullptr;
    SetResult(ns_result_);
  }
private:
  Shptr<Task> subtask_;
  EmptyHandler user_handler_;
  NsCmdResult ns_result_;
};

} // namespace {

// ------------------------------------------------------------------------------------

DummyModule::~DummyModule()
{
}

DummyModule::DummyModule(const char* debug_tag)
{
  SET_DEBUG_TAG(*this, debug_tag);
  disp_table_.AddCommand("echo-args", co::bind(&DummyModule::DispatchEchoargsCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("echo-input", co::bind(&DummyModule::DispatchEchoinputCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("wait", co::bind(&DummyModule::DispatchWaitCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("set-session-tag", co::bind(&DummyModule::DispatchSetSessionTagCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("get-session-tag", co::bind(&DummyModule::DispatchGetSessionTagCmd, this, _1, _2, _3, _4));
  disp_table_.AddCommand("exec", co::bind(&DummyModule::DispatchExecCmd, this, _1, _2, _3, _4));
// TODO: "compare-pens" (sorte);
}

// --------------------------------------------------------------------------------------------------

void DummyModule::OnSessionCreated(EngineSessionApi& sess_api,
                                          Uptr<SessionModuleData>& module_data) {
  DummySessionModuleData* new_data = new DummySessionModuleData;
  new_data->num_cmds_executed_ = 0;
  module_data = Uptr<SessionModuleData>(new_data);
}

const DispatchCmdFuncMap& DummyModule::GetDispatchCmdFuncMap() {
  return disp_table_.GetMap();
}

// ------------------------------------------------------------------------------

void DummyModule::DispatchEchoargsCmd(DispatchContext& disp_context,
                                      DispatchCmdData& disp_data,
                                      Shptr<Strand> strand,
                                      Shptr<DispatchCmdTask>& new_task) {
  DCHECK(IntermedCmdRepr(disp_data.GetNsCommand()).ProgramName() == "echo-args");
  DCHECK(new_task == nullptr);

  auto& cmd = disp_data.GetNsCommand();
  StringVector args;
  ParseCmdlineToArgv(cmd.c_str(), args);

  // Remove first part (program name). The rest are args.
  args.erase(args.begin());

  disp_data.GetNsResult() = NsCmdResult(kNsCmdExecuted, 0, NsResultType::kText).WithTextBody(args);

  auto our_data = static_cast<DummySessionModuleData*>(disp_data.GetSessionModuleData());
  our_data->num_cmds_executed_ += 1;

  // |new_task| hasn't been set means the task is immediately completed.
}

void DummyModule::DispatchEchoinputCmd(DispatchContext& disp_context,
                                       DispatchCmdData& disp_data,
                                       Shptr<Strand> strand,
                                       Shptr<DispatchCmdTask>& new_task) {
  DCHECK(IntermedCmdRepr(disp_data.GetNsCommand()).ProgramName() == "echo-input");
  DCHECK(new_task == nullptr);

  auto& cmd = disp_data.GetNsCommand();
  StringVector args;
  ParseCmdlineToArgv(cmd.c_str(), args);

  if (args.size() == 1) {
    // OK, no args
    if (disp_data.GetInputNsResult() != nullptr) {
      // Echo input result as command result
      disp_data.GetNsResult() = *disp_data.GetInputNsResult();
    }
    else {
      disp_data.GetNsResult() = NsCmdResult(kNsCmdExecuted, 0, NsResultType::kMessage)
        .WithMessageBody("This command should be executed with input result");
    }
  }
  else {
    disp_data.GetNsResult() = NsCmdResult(kNsCmdExecuted, 0, NsResultType::kMessage)
      .WithMessageBody("No args allowed, only input");
  }
  
  auto our_data = static_cast<DummySessionModuleData*>(disp_data.GetSessionModuleData());
  our_data->num_cmds_executed_ += 1;
}

void DummyModule::DispatchWaitCmd(DispatchContext& disp_context,
                                  DispatchCmdData& disp_data,
                                  Shptr<Strand> strand,
                                  Shptr<DispatchCmdTask>& new_task) {
  DCHECK(IntermedCmdRepr(disp_data.GetNsCommand()).ProgramName() == "wait");
  DCHECK(new_task == nullptr);
  // Schedule new task
  new_task = make_shared<WaitCmdTask>(
    make_shared<Strand>(disp_context.GetSessionApi().GetSessionIoContext()));
}

void DummyModule::DispatchSetSessionTagCmd(DispatchContext& disp_context,
                                           DispatchCmdData& disp_data,
                                           Shptr<Strand> strand,
                                           Shptr<DispatchCmdTask>& new_task) {
  // *** Will be zca err cat/subcategories
  DCHECK(new_task == nullptr);
  StringVector argv;
  if (ParseCmdlineToArgv(disp_data.GetNsCommand().c_str(), argv)) {
    DCHECK(argv[0] == "set-session-tag");
    if (argv.size() == 2) {
      disp_data.GetSessionModuleDataAs<DummySessionModuleData>()->session_tag_ = argv[1];
      disp_data.GetNsResult() = NsCmdResult(kNsCmdExecuted, 0);
    }
    else {
      disp_data.GetNsResult() = NsCmdResult(kNsCmdExecuted, -1).WithMessageBody("bad arg count");
    }
  }
  else {
    disp_data.GetNsResult() = NsCmdResult(kNsCmdExecuted, -1).WithMessageBody("bad cmdline");
  }

  auto our_data = static_cast<DummySessionModuleData*>(disp_data.GetSessionModuleData());
  our_data->num_cmds_executed_ += 1;
}

void DummyModule::DispatchGetSessionTagCmd(DispatchContext& disp_context,
                                           DispatchCmdData& disp_data,
                                           Shptr<Strand> strand,
                                           Shptr<DispatchCmdTask>& new_task) {
  DCHECK(new_task == nullptr);
  auto our_module_data = disp_data.GetSessionModuleDataAs<DummySessionModuleData>();
  disp_data.GetNsResult() = NsCmdResult(kNsCmdExecuted, 0).WithMessageBody(
      our_module_data->session_tag_);
  auto our_data = static_cast<DummySessionModuleData*>(disp_data.GetSessionModuleData());
  our_data->num_cmds_executed_ += 1;
}

void DummyModule::DispatchExecCmd(DispatchContext& disp_context,
                                  DispatchCmdData& disp_data,
                                  Shptr<Strand> strand,
                                  Shptr<DispatchCmdTask>& new_task) {
  // Schedule the task, Mr. Musk.
  new_task = make_shared<ExecCmdTask>(
    make_shared<Strand>(disp_context.GetSessionApi().GetSessionIoContext()));
}

}}
