#pragma once

#include "vlanbleed2/async/test_kit/stm/script_strategy_impl.h"

#include "co/async/service.h"

//
// Script Strategy For Service (runs Script strategies on co::async::Service)
//

//
// STRANDS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//

class ScriptStrategyForService : public ScriptStrategyImpl {
public:
  virtual ~ScriptStrategyForService() = default;

  using Service = co::async::Service;
  using Stream = co::async::Stream;
  using StreamAcceptor = co::async::StreamAcceptor;
  using StreamConnector = co::async::StreamConnector;

  ScriptStrategyForService(const Script& script, bool dry_run, Service& service,
                           const Endpoint& link_addr);

  co::DebugTagOwner& _DbgGetTag() { return _dbg_tag_; }

private:
  void Do_Accept(const SOp& op, RefTracker rt) override;
  void Do_Connect(const SOp& op, RefTracker rt) override;
  void Do_Write(const SOp& op, RefTracker rt) override;
  void Do_WriteAll(const SOp& op, RefTracker rt) override;
  void Do_Read(const SOp& op, RefTracker rt) override;
  void Do_ReadAll(const SOp& op, RefTracker rt) override;
  void Do_ShutdownSend(const SOp& op, RefTracker rt) override;
  void Do_Close(const SOp& op, RefTracker rt) override;

  // ...ctx-dependent operations skipped ...

  void Do_Sleep(const SOp& op, RefTracker rt) override;
  void Do_WaitEvent(const SOp& op, RefTracker rt) override;

 private:
  // [ScriptStrategyImpl impl]
  void BeginOp(const SOp& op, RefTracker rt) override;

  // Internal functions
  void BeginContextDependentOp(const SOp& op, RefTracker rt);

  void HandleAcceptOrConnect(Errcode err, bool, RefTracker rt);
  void HandleWrite(Errcode err, size_t bytes_transferred, RefTracker rt);
  void HandleRead(Errcode err, size_t bytes_transferred, RefTracker rt);

 private:
  Service& service_;
  Endpoint link_addr_;
  Errcode last_err_;
  Uptr<SOp> op_after_link_;
  Shptr<Stream> stm_sh_;
  Uptr<StreamAcceptor> stm_acpt_;
  Uptr<StreamConnector> stm_conn_;
  bool doing_op_{ false };
  Uptr<SOp> cur_write_op_;

  co::DebugTagOwner _dbg_tag_;
};


