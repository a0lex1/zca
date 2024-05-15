#pragma once

#include "vlanbleed2/async/test_kit/stm/script_strategy.h" //fixnames

//
// Common layer for connection-oriented ScriptStrategy implementation(s)
// Implements context(stream/socket)-independent part of SOps
// Derived need to override BeginOp and handle it's context-dependent codes
// first, then call us (ScriptStrategyImpl::DoOpAgain())
//

class ScriptStrategyImpl : public ScriptStrategy {
 public:
  virtual ~ScriptStrategyImpl() = default;

  using ScriptStrategy::ScriptStrategy;
  using RefTracker = co::RefTracker;
  using Endpoint = co::net::Endpoint;

  void Start(RefTracker rt) override;

  enum class Role { eNotSet, eAccept, eConnect };

  // For cases where user doesn't begins the chain with Accept(). or Connect().
  // Preset the role before start
  void SetRole(Role role);
  Role GetRole() const;
  void SetLinkDone();
  bool IsLinkDone() const;

 protected:
  void OpDone(const SOpRes& opres, RefTracker rt);
  virtual void BeginOp(const SOp& op, RefTracker rt);

 private:
  // Internal funcs
  void DoOpAgain(RefTracker rt);

 private:
  // Internal variables
  Role role_{ Role::eNotSet };
  Endpoint link_addr_;
  bool link_done_{ false };

  Uptr<SOpRes> prev_op_result_;

 private:
  // Internal funcs again
  void AssertionFailed(const SOp& op);

  // To implement
  // [ context-dependent (stream/socket read, write, etc.) ]
  virtual void Do_Accept(const SOp& op, RefTracker rt) = 0;
  virtual void Do_Connect(const SOp& op, RefTracker rt) = 0;
  virtual void Do_Write(const SOp& op, RefTracker rt) = 0;
  virtual void Do_WriteAll(const SOp& op, RefTracker rt) = 0;
  virtual void Do_Read(const SOp& op, RefTracker rt) = 0;
  virtual void Do_ReadAll(const SOp& op, RefTracker rt) = 0;
  virtual void Do_ShutdownSend(const SOp& op, RefTracker rt) = 0;
  virtual void Do_Close(const SOp& op, RefTracker rt) = 0;

  // [ context-independent ]

  void Do_ResultEQ(const SOp& op, RefTracker rt);
  void Do_ErrcodeEQ(const SOp& op, RefTracker rt);
  void Do_BytecountEQ(const SOp& op, RefTracker rt);
  void Do_BufferEQ(const SOp& op, RefTracker rt);
  void Do_BytecountLEQ(const SOp& op, RefTracker rt);
  void Do_BytecountGEQ(const SOp& op, RefTracker rt);

  // ... [ auxiliary ] ...

  virtual void Do_Sleep(const SOp& op, RefTracker rt) = 0;
  virtual void Do_WaitEvent(const SOp& op, RefTracker rt) = 0;
};


