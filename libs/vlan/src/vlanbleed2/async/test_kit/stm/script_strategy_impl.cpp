#include "vlanbleed2/async/test_kit/stm/script_strategy_impl.h" //fixnames

#include "co/xlog/xlog.h"

using namespace co;
using namespace std;

void ScriptStrategyImpl::Start(RefTracker rt)
{
  DoOpAgain(rt);
}

void ScriptStrategyImpl::SetRole(Role role)
{
  role_ = role;
}

ScriptStrategyImpl::Role ScriptStrategyImpl::GetRole() const
{
  return role_;
}

void ScriptStrategyImpl::SetLinkDone()
{
  link_done_ = true;
}

bool ScriptStrategyImpl::IsLinkDone() const
{
  return link_done_;
}

// protected

void ScriptStrategyImpl::OpDone(const SOpRes& opres, RefTracker rt)
{
  GetOpResultVector().push_back(opres);

  prev_op_result_ = make_unique<SOpRes>(opres);

  DoOpAgain(rt);
}

void ScriptStrategyImpl::BeginOp(const SOp& op, RefTracker rt)
{
  switch (op.opcode) {
  case SOpCode::Op_Invalid: NOTREACHED();
    // ... context-dependent skipped ...
  case SOpCode::Op_ResultEQ:
    return Do_ResultEQ(op, rt);
  case SOpCode::Op_ErrcodeEQ:
    return Do_ErrcodeEQ(op, rt);
  case SOpCode::Op_BytecountEQ:
    return Do_BytecountEQ(op, rt);
  case SOpCode::Op_BufferEQ:
    return Do_BufferEQ(op, rt);
  case SOpCode::Op_BytecountLEQ:
    return Do_BytecountLEQ(op, rt);
  case SOpCode::Op_BytecountGEQ:
    return Do_BytecountGEQ(op, rt);
    // ... context-dependent skipped ...
  default: NOTREACHED();
  }
}

// private

void ScriptStrategyImpl::DoOpAgain(RefTracker rt)
{
  SOp op;
  if (GetNextOp(op)) {
    BeginOp(op, rt);
  }
  else {
    // no more ops, script finished
    syslog(_INFO) << "Script Finished\n";
  }
}

// Internal funcs again

void ScriptStrategyImpl::AssertionFailed(const SOp& op)
{
  syslog(_FATAL) << "Script Assertion Failed ::: opcode " << (int)op.opcode << "\n";
  syslog(_FATAL) << "Textualized OP:\n";
  syslog(_FATAL) << op.GetTextualized() << "\n\n";
  DCHECK(!"I kill you");
}

// [ context-independent ]

void ScriptStrategyImpl::Do_ResultEQ(const SOp& op, RefTracker rt)
{
  DCHECK(prev_op_result_); // todo
  // Compare op's arg with previous result
  if (*prev_op_result_.get() != op.arguments.GetOpResultArg()) {
    if (!IsDryRun()) {

      syslog(_ERR) << "\n\n****************************\n"
        "SCRIPT STRATEGY: ResultEQ failed\n***********************\n";

      syslog(_ERR) << "Prev OP result:\n";
      syslog(_ERR) << prev_op_result_->GetTextualized() << "\n\n";

      syslog(_ERR) << "Expected result:\n";
      syslog(_ERR) << op.arguments.GetOpResultArg().GetTextualized() << "\n\n";

      syslog(_ERR) << "********************************\n\n";

      AssertionFailed(op);
    }
    else {
      // dry run, ignore
    }
  }
}

void ScriptStrategyImpl::Do_ErrcodeEQ(const SOp& op, RefTracker rt)
{
  DCHECK(prev_op_result_); // todo
  // Meaningless to compare results that doesn't have errcode
  // DCHECK such flag it for both us and |op|
  DCHECK(prev_op_result_->flags.has_errcode);
  DCHECK(op.arguments.GetOpResultArg().flags.has_errcode);
  if (prev_op_result_->errcode != op.arguments.GetOpResultArg().errcode) {
    AssertionFailed(op);
  }
}

void ScriptStrategyImpl::Do_BytecountEQ(const SOp& op, RefTracker rt)
{
  DCHECK(prev_op_result_); // todo
  // ...
  DCHECK(prev_op_result_->flags.has_bytecount);
  DCHECK(op.arguments.GetOpResultArg().flags.has_bytecount);
  if (prev_op_result_->bytecount != op.arguments.GetOpResultArg().bytecount) {
    AssertionFailed(op);
  }
}

void ScriptStrategyImpl::Do_BufferEQ(const SOp& op, RefTracker rt)
{
  DCHECK(prev_op_result_); // todo
  DCHECK(prev_op_result_->flags.has_buffer);
  DCHECK(op.arguments.GetOpResultArg().flags.has_buffer);
  if (prev_op_result_->buffer != op.arguments.GetOpResultArg().buffer) {
    AssertionFailed(op);
  }
}

void ScriptStrategyImpl::Do_BytecountLEQ(const SOp& op, RefTracker rt)
{
  DCHECK(prev_op_result_); // todo
  DCHECK(prev_op_result_->flags.has_bytecount);
  DCHECK(op.arguments.GetOpResultArg().flags.has_bytecount);
  if (!(prev_op_result_->bytecount <= op.arguments.GetOpResultArg().bytecount)) {
    AssertionFailed(op);
  }
}

void ScriptStrategyImpl::Do_BytecountGEQ(const SOp& op, RefTracker rt)
{
  DCHECK(prev_op_result_); // todo
  DCHECK(prev_op_result_->flags.has_bytecount);
  DCHECK(op.arguments.GetOpResultArg().flags.has_bytecount);
  if (!(prev_op_result_->bytecount >= op.arguments.GetOpResultArg().bytecount)) {
    AssertionFailed(op);
  }
}



