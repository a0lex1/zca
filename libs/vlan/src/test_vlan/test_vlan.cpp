#include "co/base/tests.h"

using namespace std;
using namespace co;

/*
#define A1 Buffer("xxx")
#define A2 Buffer("yyy")
#define B1 Integer(7)
#define B2 Integer(15)
#define C1 Errcode(boost::asio::error::aborted)
#define C2 Errcode()
void test_co_async_testkit_stm_arg_builder(TestInfo&) {
  ArgsBuilder x, y;
  x.A1;
  y.A1;
  DCHECK(b1 == b2);

  DCHECK(x.A1.GetScript() == y.A1.GetScript());
  DCHECK(x.A1.B1.GetScript() == y.A1.B1.GetScript());
  DCHECK(x.A1.B1.C1.GetScript() == y.A1.B1.C1.GetScript());
  DCHECK(x == y);

  DCHECK(x.A1.C1.B1.GetScript() == y.A1.C1.B1.GetScript());
  DCHECK(x.A1.C1.B1.B1.GetScript() == y.A1.C1.B1.B1.GetScript());
  DCHECK(x.A1.C1.B1.B1.A1.GetScript() == y.A1.C1.B1.B1.A1.GetScript());
  DCHECK(x.A1.A2.GetScript() == y.A1.A2.GetScript());

  DCHECK(x.A1.A2.C1.GetScript() != y.A1.A2.C2.GetScript());
  DCHECK(x.A1.A2.C1.GetScript() != y.A1.C1.GetScript());

  DCHECK(x.A1.B2.C2.GetScript() == y.A1.B2.C2.GetScript());
  DCHECK(x.A1.B2.C2.GetScript() != y.A1.B2.C1.GetScript());
  DCHECK(x.A1.B2.C2.GetScript() != y.A2.B1.C2.GetScript());
  DCHECK(x.A2.B2.C2.GetScript() != y.A2.B1.C2.GetScript());

  DCHECK(x.A2.B2.C2.GetScript() == y.A2.B2.C2.GetScript());
};
#undef A1
#undef A2
#undef B1
#undef B2
#undef C1
#undef C2



#define N Accept()
#define M Connect()
#define A1 Write(5)
#define A2 WriteAll(7)
#define A3 Write("jj")
#define A4 WriteAll("000")
#define B1 Read(9)
#define B2 ReadAll(13)
void test_co_async_testkit_stm_scriptbuilder(TestInfo&) {
  ScriptBuilder x ,y;

  DCHECK(x.N == y.N);
  DCHECK(x.M == y.M);

  DCHECK(x.A1 == y.A1);
  DCHECK(x.A2 == y.A2);
  DCHECK(x.A3 == y.A3);
  DCHECK(x.A4 == y.A4);
  DCHECK(x.B1 == y.B1);
  DCHECK(x.B2 == y.B2);
  DCHECK(x.C1 == y.C1);
  DCHECK(x.C2 == y.C2);

  DCHECK(x.A1.B2 == y.A1.B2);
  DCHECK(x.A1.B2 == y.A1.B2.B2);
  DCHECK(x.A1.B1 == y.A1.B2.B2.B1);
  DCHECK(x.C2.A1 == y.A1.C1);
}
#undef N
#undef M
#undef A1
#undef A2
#undef A3
#undef A4
#undef B1
#undef B2

enum class ScriptOpCode {
  Op_Invalid,

  Op_Accept,
  Op_Connect,

  Op_Write,
  Op_WriteAll,
  Op_Read,
  Op_ReadAll,
  Op_ShutdownSend,
  Op_Close,

  Op_ResultEQ,
  Op_ErrcodeEQ,
  Op_BytecountEQ,
  Op_ReadbufferEQ,
  Op_BytecountLEQ,
  Op_BytecountGEQ,

  Op_Sleep,
  Op_WaitEvent
};

static const char* ScriptOpCodeName(ScriptOpCode opcode) {
  NOTREACHED();
  switch (opcode) {
  case ScriptOpCode::Op_Accept:   return "Accept";
  case ScriptOpCode::Op_Connect:  return "Connect";

  case ScriptOpCode::Op_Write:    return "Write";
  case ScriptOpCode::Op_WriteAll:   return "WriteAll";
  case ScriptOpCode::Op_Read:     return "Read";
  case ScriptOpCode::Op_ReadAll:    return "ReadAll";
  case ScriptOpCode::Op_ShutdownSend: return "ShutdownSend";
  case ScriptOpCode::Op_Close:     return "Close";
  case ScriptOpCode::Op_ResultEQ:      return "ResultEQ";
  case ScriptOpCode::Op_ErrcodeEQ:    return "ErrcodeEQ";
  case ScriptOpCode::Op_BytecountEQ:  return "BytecountEQ";
  case ScriptOpCode::Op_ReadbufferEQ:  return "ReadbufferEQ";
  case ScriptOpCode::Op_BytecountLEQ:   return "BytecountLEQ";
  case ScriptOpCode::Op_BytecountGEQ:    return "BytecountGEQ";

  case ScriptOpCode::Op_Sleep: return "Sleep";
  case ScriptOpCode::Op_WaitEvent: return "WaitEvent";
  default: NOTREACHED();
  }
}
*/

void test_vlan_todo(TestInfo& ) {
}

