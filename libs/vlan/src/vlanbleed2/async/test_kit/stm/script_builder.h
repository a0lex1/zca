#pragma once

#include "vlanbleed2/async/test_kit/stm/script.h"

//
// Script Builder (Helps().ToBuild().ScriptOp().Chains())
//

class SBuilder
{
  Script script_;
 public:
  Script& S() { return script_; }

  SBuilder& Accept() {
    script_.AddOp(SOp(SOpCode::Op_Accept));
    return *this;
  }
  SBuilder& Connect() {
    script_.AddOp(SOp(SOpCode::Op_Connect));
    return *this;
  }

  // IO

  SBuilder& Write(const std::string& buf) {
    script_.AddOp(SOp(SOpCode::Op_Write, SOpArgs(buf)));
    return *this;
  }
  SBuilder& WriteAll(const std::string& buf) {
    script_.AddOp(SOp(SOpCode::Op_WriteAll, SOpArgs(buf)));
    return *this;
  }
  SBuilder& Read(size_t count) {
    script_.AddOp(SOp(SOpCode::Op_Read, SOpArgs(count)));
    return *this;
  }
  SBuilder& ReadAll(size_t count) {
    script_.AddOp(SOp(SOpCode::Op_ReadAll, SOpArgs(count)));
    return *this;
  }
  SBuilder& ShutdownSend() {
    script_.AddOp(SOp(SOpCode::Op_ShutdownSend));
    return *this;
  }
  SBuilder& Close() {
    script_.AddOp(SOp(SOpCode::Op_Close));
    return *this;
  }

  // EQ

  SBuilder& ResultEQ(const SOpRes& expected_result) {
    script_.AddOp(SOp(SOpCode::Op_ResultEQ, SOpArgs(expected_result)));
    return *this;
  }
  SBuilder& ErrcodeEQ(Errcode err) {
    script_.AddOp(SOp(SOpCode::Op_ErrcodeEQ, SOpArgs(Errcode(err))));
    return *this;
  }
  SBuilder& BytecountEQ(size_t bytecount) {
    script_.AddOp(SOp(SOpCode::Op_BytecountEQ, SOpArgs(bytecount)));
    return *this;
  }
  SBuilder& BufferEQ(const std::string& expected_buf) {
    script_.AddOp(SOp(SOpCode::Op_BufferEQ, SOpArgs(expected_buf)));
    return *this;
  }
  SBuilder& BytecountLEQ(size_t bytecount) {
    script_.AddOp(SOp(SOpCode::Op_BytecountLEQ, SOpArgs(bytecount)));
    return *this;
  }
  SBuilder& BytecountGEQ(size_t bytecount) {
    script_.AddOp(SOp(SOpCode::Op_BytecountGEQ, SOpArgs(bytecount)));
    return *this;
  }
  SBuilder& Sleep(size_t msec) {
    script_.AddOp(SOp(SOpCode::Op_Sleep, SOpArgs(msec)));
    return *this;
  }
  SBuilder& WaitEvent(const std::string& event_name, size_t msec) {
    script_.AddOp(SOp(SOpCode::Op_WaitEvent, SOpArgs(event_name, msec)));
    return *this;
  }

  // shortcuts

  SBuilder& NoErr() {
    return ErrcodeEQ(co::NoError());
  }
  SBuilder& Write(size_t bytecount) {
    return Write(std::string(bytecount, 'k'));
  }
  SBuilder& WriteAll(size_t bytecount) {
    return WriteAll(std::string(bytecount, 'l'));
  }
};


