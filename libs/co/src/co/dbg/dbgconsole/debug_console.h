#pragma once

#include "co/dbg/dbgconsole/debug_console_module.h"

#include "co/async/server.h"
#include "co/async/loop_object.h"

namespace co {
namespace dbg {
namespace dbgconsole {

class DebugConsole: public co::async::LoopObjectNoreset {
public:
  virtual ~DebugConsole() = default;
  
  void Configure(const co::net::Endpoint& locaddr, Shptr<co::async::Service> svc);

  // only after Configure():
  void AddModule(Uptr<DebugConsoleModule> module);

  // only after Configure():
  void PrepareToStart(Errcode&) override;
  void Start(RefTracker rt) override;
  void StopThreadsafe() override;

  co::net::Endpoint GetLocalAddressToConnect() { return srv_->GetLocalAddressToConnect(); }

  void CleanupAbortedStop() override;
  
private:
  Shptr<co::async::Session> SrvSessFacFunc(Uptr<co::async::Stream>, Shptr<Strand>);

private:
  bool configured_;
  Uptr<co::async::ServerWithSessList> srv_;
  std::vector<Uptr<DebugConsoleModule>> modules_;
};




}}}
