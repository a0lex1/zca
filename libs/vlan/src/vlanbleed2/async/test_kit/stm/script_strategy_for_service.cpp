#include "vlanbleed2/async/test_kit/stm/script_strategy_for_service.h"

#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace boost::asio;

ScriptStrategyForService::ScriptStrategyForService(const Script& script, bool dry_run, Service& service, const Endpoint& link_addr) :
  ScriptStrategyImpl(script, dry_run),
  service_(service),
  link_addr_(link_addr)
{

}

void ScriptStrategyForService::Do_Accept(const SOp& op, RefTracker rt)
{
  DCHECK(!stm_acpt_); // no second accept
  DCHECK(!stm_sh_);
  DCHECK(!IsLinkDone());
  SetRole(Role::eAccept);
  stm_acpt_ = service_.CreateStreamAcceptorFactory()->CreateStreamAcceptor();
  // Throws
  stm_acpt_->Open();
  stm_acpt_->Bind(link_addr_);
  stm_acpt_->StartListening();
  Uptr<Stream> stm = service_.CreateStreamFactory()->CreateStream();
  // move Uptr -> Shptr
  stm_sh_ = move(stm);
  doing_op_ = true;
  stm_acpt_->AsyncAccept(*stm_sh_.get(),
                         co::bind(&ScriptStrategyForService::HandleAcceptOrConnect,
                         this, _1, true, rt));
}

void ScriptStrategyForService::Do_Connect(const SOp& op, RefTracker rt)
{
  DCHECK(!stm_conn_); // no second connect
  DCHECK(!stm_sh_);
  DCHECK(!IsLinkDone());
  SetRole(Role::eConnect);
  stm_conn_ = service_.CreateStreamConnectorFactory()->CreateStreamConnector();
  Uptr<Stream> stm = service_.CreateStreamFactory()->CreateStream();
  stm_sh_ = move(stm);
  doing_op_ = true;
  stm_conn_->AsyncConnect(link_addr_, *stm_sh_.get(),
                          co::bind(&ScriptStrategyForService::HandleAcceptOrConnect,
                            this, _1, false, rt));
}

void ScriptStrategyForService::Do_Write(const SOp& op, RefTracker rt)
{
  DCHECK(IsLinkDone());
  DCHECK(stm_sh_);
  DCHECK(!cur_write_op_);

  // Clone and save this op, we're writing it -> need to keep buffer
  cur_write_op_ = make_unique<SOp>(op);
  auto buf = const_buffers_1(cur_write_op_->arguments.GetStringArg().c_str(),
                             cur_write_op_->arguments.GetStringArg().size());
  doing_op_ = true;
  stm_sh_->AsyncWriteSome(buf,
                          co::bind(&ScriptStrategyForService::HandleWrite,
                            this, _1, _2, rt));
}

void ScriptStrategyForService::Do_WriteAll(const SOp& op, RefTracker rt)
{
  DCHECK(IsLinkDone());
  DCHECK(stm_sh_);
}

void ScriptStrategyForService::Do_Read(const SOp& op, RefTracker rt)
{
  DCHECK(IsLinkDone());
  DCHECK(stm_sh_);
}

void ScriptStrategyForService::Do_ReadAll(const SOp& op, RefTracker rt)
{
  DCHECK(IsLinkDone());
  DCHECK(stm_sh_);
}

void ScriptStrategyForService::Do_ShutdownSend(const SOp& op, RefTracker rt)
{
  DCHECK(IsLinkDone());
  DCHECK(stm_sh_);
  Errcode err;
  stm_sh_->Shutdown(err);
  OpDone(SOpRes(err), rt);
}

void ScriptStrategyForService::Do_Close(const SOp& op, RefTracker rt)
{
  DCHECK(IsLinkDone());
  DCHECK(stm_sh_);
  Errcode err;
  stm_sh_->Close(); //????? errcode?
  OpDone(SOpRes(co::NoError()), rt);
}

void ScriptStrategyForService::Do_Sleep(const SOp& op, RefTracker rt)
{
  NOTREACHED();
}

void ScriptStrategyForService::Do_WaitEvent(const SOp& op, RefTracker rt)
{
  NOTREACHED();
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ScriptStrategyForService::BeginOp(const SOp& op, RefTracker rt)
{
  if (IsLinkDone()) {

    // Parties are already connected. All we need is just to do the requested |op|.

    syslog(_DBG)
      << "[scriptStrat " << GET_DEBUG_TAG(_dbg_tag_) << "] <OP> " << ScriptOpCodeName(op.opcode)
      << "(" << op.arguments.GetTextualized() << ")"
      << "\n";

    BeginContextDependentOp(op, rt);
  }
  else {

    // Parties are not yet connected.

    // If it's a second op when the role isn't yet assigned = assert............??
    DCHECK(!last_err_);

    // Will be done after successful accept/connect
    op_after_link_ = make_unique<SOp>(op);
    switch (GetRole()) {
    case ScriptStrategyImpl::Role::eConnect: {
      // Make Connect op and do it first, then |op|
      SOp conn_op(SOpCode::Op_Connect, SOpArgs(link_addr_));

      syslog(_DBG)
        << "[scriptStrat " << GET_DEBUG_TAG(_dbg_tag_) << "] <OP> " << ScriptOpCodeName(op.opcode)
        << "{auto-connect to " << link_addr_.ToString() << "} (" << op.arguments.GetTextualized() << ")"
        << "\n";

      Do_Connect(conn_op, rt);
      break;
    }
    case ScriptStrategyImpl::Role::eAccept: {
      // Make Accept op and do it first, then |op|
      SOp acpt_op(SOpCode::Op_Connect, SOpArgs(link_addr_));

      syslog(_DBG)
        << "[scriptStrat " << GET_DEBUG_TAG(_dbg_tag_) << "] <OP> " << ScriptOpCodeName(op.opcode)
        << "{auto-accept on " << link_addr_.ToString() << "} (" << op.arguments.GetTextualized() << ")"
        << "\n";

      Do_Accept(acpt_op, rt);
      break;
    }
    case ScriptStrategyImpl::Role::eNotSet:
      NOTREACHED();
    default: NOTREACHED();
    }
  }
}

void ScriptStrategyForService::BeginContextDependentOp(const SOp& op, RefTracker rt)
{
  switch (op.opcode) {
  case SOpCode::Op_Accept:
    return Do_Accept(op, rt);
  case SOpCode::Op_Connect:
    return Do_Connect(op, rt);

  case SOpCode::Op_Write:
    return Do_Write(op, rt);
  case SOpCode::Op_WriteAll:
    return Do_WriteAll(op, rt);
  case SOpCode::Op_Read:
    return Do_Read(op, rt);
  case SOpCode::Op_ReadAll:
    return Do_ReadAll(op, rt);

  case SOpCode::Op_ShutdownSend:
    return Do_ShutdownSend(op, rt);
  case SOpCode::Op_Close:
    return Do_Close(op, rt);
    // ... context-independent skipped ...
  case SOpCode::Op_Sleep:
    return Do_Sleep(op, rt);
  case SOpCode::Op_WaitEvent:
    return Do_WaitEvent(op, rt);
  }
  // Not our Op, try handle in base class
  ScriptStrategyImpl::BeginOp(op, rt);
}

void ScriptStrategyForService::HandleAcceptOrConnect(Errcode err,
                                                     bool accept_not_connect,
                                                     RefTracker rt)
{
  DCHECK(doing_op_);
  doing_op_ = false;

  // We are unable to establish link (connect/accept), we can't run the script.
  // This is not a script error, this is script runner error.
  if (err) {
    const string s = accept_not_connect ? "accepting" : "connecting";
    syslog(_ERR) << "Error " << err << " while " << s << "\n";
    return;
  }
  if (op_after_link_) {
    // we are automatic operation, don't include us to log (don't call OpDone())
  }
  else {
    OpDone(SOpRes(err, 0, ""), rt);
  }
  SetLinkDone();
  if (op_after_link_) {
    BeginContextDependentOp(*op_after_link_.get(), rt);
    op_after_link_ = nullptr;
  }
}

void ScriptStrategyForService::HandleWrite(Errcode err, size_t bytes_transferred,
                                           RefTracker rt)
{
  DCHECK(doing_op_);
  DCHECK(cur_write_op_);
  doing_op_ = false;

  // Don't check |err|. Do OpDone() anyway.
  // It shall be reentrancy so call before calling handler. We're not win32k, right?
  auto opcopy(*cur_write_op_.get());
  cur_write_op_ = nullptr;
  const string& write_buf_was = opcopy.arguments.GetStringArg();
  OpDone(SOpRes(err, bytes_transferred, write_buf_was.c_str()), rt);
}

