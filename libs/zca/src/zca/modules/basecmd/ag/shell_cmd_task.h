#pragma once

#include "zca/engine/sdk/dispatch_cmd_task.h"

#include "co/async/process_io_redirector.h"

namespace modules {
namespace basecmd {
namespace ag {

class ShellCmdTask : public engine::DispatchCmdTask,
  public co::enable_shared_from_this<ShellCmdTask>
{
public:
  virtual ~ShellCmdTask() = default;

  using RefTracker = co::RefTracker;

  ShellCmdTask(Shptr<Strand> strand);

private:
  void BeginIo(RefTracker) override;

  void HandleTimeout(Errcode, RefTracker);
  void ReadOutputAgain(RefTracker);
  void HandleReadOutput(Errcode, size_t, RefTracker);
  void FixUnprintableChars();

  void StopUnsafeExtra() override;

private:
  int timeout_secs_{ -1 };
  boost::asio::deadline_timer timer_;
  co::async::ProcessIoRedirector redirector_;
  char portion_buffer_[8192];
  std::string final_buffer_;
};

}}}


