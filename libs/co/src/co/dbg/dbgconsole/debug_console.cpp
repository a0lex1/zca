#include "co/dbg/dbgconsole/debug_console.h"
#include "co/dbg/dbgconsole/detail/debug_console_server_session.h"

#include "co/base/cmdline/parsed_command_line.h"

using namespace co::async;

namespace co {
namespace dbg {
namespace dbgconsole {

void DebugConsole::Configure(const co::net::Endpoint& locaddr, Shptr<Service> svc) {
  DCHECK(!configured_);
  srv_ = make_unique<ServerWithSessList>(locaddr, svc, co::bind(&DebugConsole::SrvSessFacFunc, this, _1, _2));
  configured_ = true;
}

void DebugConsole::AddModule(Uptr<DebugConsoleModule> module) {
  DCHECK(configured_);
  modules_.emplace_back(move(module));
}

void DebugConsole::CleanupAbortedStop() {
  srv_->CleanupAbortedStop();
}

Shptr<Session> DebugConsole::SrvSessFacFunc(Uptr<Stream> new_stream,
  Shptr<Strand> session_strand)
{
  return std::make_shared<detail::DebugConsoleServerSession>(
    move(new_stream),
    session_strand,
    modules_);
}

void DebugConsole::PrepareToStart(Errcode& err) {
  srv_->PrepareToStart(err);

}

void DebugConsole::Start(RefTracker rt) {
  srv_->Start(rt);
}

void DebugConsole::StopThreadsafe() {
  if (srv_) {
    srv_->StopThreadsafe();
  }
}

}}}
