#include "co/dbg/dbgconsole/detail/debug_console_server_session.h"

#include "co/async/wrap_post.h"
#include "co/base/cmdline/parsed_command_line.h"
#include "co/common_config.h"

#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;

#define llog() syslog(_DBG) << "DbgConSrvSess (base at " << SPTR(static_cast<SessionBase*>(this)) << ") "

namespace co {
namespace dbg {
namespace dbgconsole {
namespace detail {

DebugConsoleServerSession::DebugConsoleServerSession(
  Uptr<co::async::Stream> new_stm,
  Shptr<Strand> strand,
  std::vector<Uptr<DebugConsoleModule>>& modules)
  :
  Session(move(new_stm), strand),
  modules_(modules),
  line_rdr_(GetStream(), co::common_config::kLineReaderMaxLineLen),
  line_writ_(GetStream())
{
}

DebugConsoleServerSession::~DebugConsoleServerSession() {
  llog() << "DTOR\n";
}

void DebugConsoleServerSession::BeginIo(RefTracker rt) {
  rt.SetReferencedObject(shared_from_this());
  ReadLineAgain(rt);
}

void DebugConsoleServerSession::ReadLineAgain(RefTracker rt) {
  line_rdr_.AsyncReadLine(cur_read_line_,
    wrap_post(GetFiberStrand(), co::bind(&DebugConsoleServerSession::HandleReadLine,
      shared_from_this(), _1, rt)));
}

void DebugConsoleServerSession::StopUnsafe() {
  // change nothing
  Session::StopUnsafe();
}

void DebugConsoleServerSession::HandleReadLine(Errcode err, RefTracker rt) {
  DCHECK(IsInsideFiberStrand());
  if (err) {
    llog() << "HandleReadLine ERR " << err << "...\n";
    return;
  }
  else {
    llog() << "HandleReadLine: `" << cur_read_line_ << "`\n";
  }

  bool abort_read = false;

  std::string output;

  cmdline::ParsedCommandLine pcl(cur_read_line_.c_str());
  if (pcl.GetParserRet() == 0) {
    if (pcl.GetArgc() > 0) {
      DispatchFunc dispfn = FindDispatcherForCommand(pcl.GetArgv()[0]);
      if (dispfn) {
        dispfn(pcl.GetArgv()[0], output, abort_read);
      }
      else {
        output = "unknown command\n";
      }
    }
    else {
      output = "!(pcl.GetArgc() > 0)\n";
    }
  }
  else {
    output = "pcl.GetParserRet() != 0, ERROR!\n";
  }

  Response(output, abort_read, rt);
}


DispatchFunc DebugConsoleServerSession::FindDispatcherForCommand(
  const std::string& cmd_name)
{
  for (auto& module : modules_) {
    const DispatchMap& module_cmd_map = module->GetDispatchMap();
    for (const auto& map_entry : module_cmd_map) {
      if (map_entry.first == cmd_name) {
        return map_entry.second;
      }
    }
  }
  return nullptr;
}

void DebugConsoleServerSession::Response(
  const std::string& out_buffer, bool abort_read,
  RefTracker rt)
{
  line_writ_.AsyncWriteLine(out_buffer,
    wrap_post(GetFiberStrand(), co::bind(&DebugConsoleServerSession::HandleWriteResponseLine,
      shared_from_this(), _1, _2, abort_read, rt)));
}

void DebugConsoleServerSession::HandleWriteResponseLine(
  Errcode err, size_t bytes_written, bool abort_read, RefTracker rt)
{
  if (err) {
    return;
  }
  if (abort_read) {
    return;
  }
  ReadLineAgain(rt);
}

}}}}
