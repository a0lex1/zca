#pragma once

#include "co/common.h"

enum class SOpCode {
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
  Op_BufferEQ,
  Op_BytecountLEQ,
  Op_BytecountGEQ,

  Op_Sleep,
  Op_WaitEvent
};

static const char* ScriptOpCodeName(SOpCode opcode) {

  switch (opcode) {

  case SOpCode::Op_Invalid: return "Invalid";

  case SOpCode::Op_Accept: return "Accept";
  case SOpCode::Op_Connect: return "Connect";

  case SOpCode::Op_Write: return "Write";
  case SOpCode::Op_WriteAll: return "WriteAll";
  case SOpCode::Op_Read: return "Read";
  case SOpCode::Op_ReadAll: return "ReadAll";
  case SOpCode::Op_ShutdownSend: return "ShutdownSend";
  case SOpCode::Op_Close: return "Close";

  case SOpCode::Op_ResultEQ: return "ResultEQ";
  case SOpCode::Op_ErrcodeEQ: return "ErrcodeEQ";
  case SOpCode::Op_BytecountEQ: return "BytecountEQ";
  case SOpCode::Op_BufferEQ: return "BufferEQ";
  case SOpCode::Op_BytecountLEQ: return "BytecountLEQ";
  case SOpCode::Op_BytecountGEQ: return "BytecountGEQ";

  case SOpCode::Op_Sleep: return "Sleep";
  case SOpCode::Op_WaitEvent: return "WaitEvent";
  }

  NOTREACHED();
  return nullptr;
};

