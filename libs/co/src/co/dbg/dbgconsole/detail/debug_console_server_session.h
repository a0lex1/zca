#pragma once

#include "co/dbg/dbgconsole/debug_console_module.h"

#include "co/async/stream_line_reader.h"
#include "co/async/stream_line_writer.h"
#include "co/async/session.h"

#include <boost/enable_shared_from_this.hpp>

namespace co {
namespace dbg {
namespace dbgconsole {
namespace detail {

class DebugConsoleServerSession
  : public co::enable_shared_from_this<DebugConsoleServerSession>,
    public co::async::Session
{
public:
  virtual ~DebugConsoleServerSession();

  DebugConsoleServerSession(
      Uptr<co::async::Stream> new_stm,
      Shptr<Strand> strand,
      std::vector<Uptr<DebugConsoleModule>>& modules
    );

private:
  void BeginIo(co::RefTracker) override;
  void StopUnsafe() override;

  void ReadLineAgain(RefTracker);
  void HandleReadLine(Errcode, RefTracker);
  DispatchFunc FindDispatcherForCommand(const std::string& cmd_name);
  void Response(const std::string&, bool abort_read, RefTracker);
  void HandleWriteResponseLine(Errcode err, size_t bytes_written, bool abort_read, RefTracker);

private:
  std::vector<Uptr<DebugConsoleModule>>& modules_;
  co::async::StreamLineReader line_rdr_;
  co::async::StreamLineWriter line_writ_;
  std::string cur_read_line_;

};


}}}}



