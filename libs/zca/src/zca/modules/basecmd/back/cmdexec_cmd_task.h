#pragma once

#include "zca/modules/basecmd/back/basecmd_backend_module.h"

#include "zca/engine/sdk/dispatch_cmd_task.h"

#include "cc/bot_id.h"

#include "co/xlog/define_logger_sink.h"

namespace modules {
namespace basecmd {
namespace back {

DECLARE_XLOGGER_SINK("cmdexeccmdtask", gZcaCmdexecCmdTaskSink);

class CmdexecCmdTask : public engine::DispatchCmdTask,
  public co::enable_shared_from_this<CmdexecCmdTask>
{
public:
  virtual ~CmdexecCmdTask() = default;

  CmdexecCmdTask(Shptr<Strand> strand);

private:
  void BeginIo(RefTracker) override;
  void StopUnsafeExtra() override;

  void BotListAccessCallback(cc::ICcBotList&, RefTracker);
  void HandleExecuteSequencedCommand(Errcode, RefTracker);
  void HandleExecutePostCommand(RefTracker);

private:
  Shptr<cc::ICcBot> bot_; // we reference the bot during all the stages
  BasecmdBotModuleData* bmdata_{ nullptr }; // keep for simplicity
  StringVector side_sections_;
  cc::BotId bot_id_;
  StringVector sig_sections_;
  std::string bot_cmdline_;
  std::string cmd_result_opaque_buf_;
  netshell::NsCmdResult unsered_result_;
  netshell::NsCmdResult post_result_;
};

}}}


