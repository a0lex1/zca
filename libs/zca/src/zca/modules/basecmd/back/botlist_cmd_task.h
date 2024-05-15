#pragma once

#include "zca/modules/basecmd/back/basecmd_backend_module.h"

#include "zca/engine/sdk/dispatch_cmd_task.h"

#include "zca/core/bot_filter.h"
#include "zca/core/field_filter.h"

namespace modules {
namespace basecmd {
namespace back {

class BotlistCmdTask : public engine::DispatchCmdTask,
  public co::enable_shared_from_this<BotlistCmdTask>
{
public:
  virtual ~BotlistCmdTask() = default;

  BotlistCmdTask(Shptr<Strand> strand);

private:
  void BeginIo(co::RefTracker) override;

  void HandleExecuteBotListAccessCallback(cc::ICcBotList& bot_list, co::RefTracker);
  void StopUnsafeExtra() override;

private:
  bool no_header_{ false };
  bool csv_{ false };
  size_t max_line_length_{ 30 };
  size_t max_line_count_{ 5 };
  core::FieldFilter field_filter_;
  core::BotFilter bot_filter_;
};

}}}


