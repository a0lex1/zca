#pragma once

#include "proto/proto_message.h"
#include "proto/proto_error.h"

#include "co/base/error.h"
#include "co/common.h"

#include <sstream>

namespace cc {

enum class CcErrc {
  ok = 0,
  transport_error_hwscm = 3910,
  unknown_proto_message_from_client = 3911,
  unexpected_command_result = 3912,
  bot_id_in_use = 3913,
  handshake_during_handshake = 3914,
  second_handshake = 3915,
  transport_error_hwhr = 3916,
  unknown_proto_message_from_server = 3917,
  proto_read_error = 3918,
  command_while_command = 3919,
  seqcmd_aborted = 3920,
  hshake_aborted = 3921,
  hshake_reply_expected = 3922,
  proto_read_error_hshake_result = 3923,
  write_failed = 3924,
  transport_error_hwcr = 3925,
};

class CcErrorInfo {
public:
  CcErrorInfo() {}
  CcErrorInfo(Errcode some_err) : some_err_(some_err)
  {
  }
  CcErrorInfo(ProtoMessageCode code) : msgcode_(code)
  {
  }
  CcErrorInfo(ProtoError pt_err) : pt_err_(pt_err) {}
  CcErrorInfo(std::string string_val): strval_(string_val) {}

  Errcode GetErrcode() const { return some_err_; }
  ProtoMessageCode GetMsgCode() const { return msgcode_; }
  ProtoError GetProtoError() const { return pt_err_;}
  const std::string& GetStringVal() const { return strval_; }

private:
  Errcode some_err_;
  ProtoMessageCode msgcode_;
  ProtoError pt_err_;
  std::string strval_;
};

class CcError : public co::Error<CcErrc, CcErrorInfo> {
 public:
  virtual ~CcError() = default;

  using Error::Error;

  static CcError NoError() { return CcError(); }

  const char* GetErrcTitle() const {
    switch (GetErrc()) {
    case CcErrc::ok: return DefaultErrcTitleOk();
    case CcErrc::transport_error_hwscm: return "transport error hwscm";
    case CcErrc::unknown_proto_message_from_client: return "unknown proto message from client";
    case CcErrc::unexpected_command_result: return "unexpected command result";
    case CcErrc::bot_id_in_use: return  "id in use";
    case CcErrc::handshake_during_handshake: return "hshake during hshake";
    case CcErrc::second_handshake: return  "second hshake";
    case CcErrc::transport_error_hwhr: return  "transport error hwhr";
    case CcErrc::unknown_proto_message_from_server: return "unknown proto message from server";
    case CcErrc::proto_read_error: return "proto read error";
    case CcErrc::command_while_command: return "command while command";
    case CcErrc::seqcmd_aborted: return "seqcmd aborted";
    case CcErrc::hshake_aborted: return "handshake aborted";
    case CcErrc::hshake_reply_expected: return "handshake reply message expected";
    case CcErrc::write_failed: return "write failed";
    case CcErrc::transport_error_hwcr: return "transport error hwcr";
    default: return DefaultErrcTitleUnknown();
    }
  }
  std::string MakeErrorMessage() const {
    std::stringstream ss;
    switch (GetErrc()) {
    case CcErrc::ok: return DefaultErrcTitleOk();
    case CcErrc::transport_error_hwscm:
      ss << GetErrcTitle() << " err " << GetErrorInfo().GetErrcode();
      break;
    case CcErrc::unknown_proto_message_from_client:
      ss << GetErrcTitle() << " msgcode " << GetErrorInfo().GetMsgCode();
      break;
    case CcErrc::unexpected_command_result:
      ss << GetErrcTitle() << " msgcode " << GetErrorInfo().GetMsgCode();
      break;
    case CcErrc::bot_id_in_use:
      ss << GetErrcTitle() << " msgcode " << GetErrorInfo().GetMsgCode();
      break;
    case CcErrc::handshake_during_handshake:
      ss << GetErrcTitle() << " msgcode " << GetErrorInfo().GetMsgCode();
      break;
    case CcErrc::second_handshake:
      ss << GetErrcTitle() << " msgcode " << GetErrorInfo().GetMsgCode();
      break;
    case CcErrc::transport_error_hwhr:
      ss << GetErrcTitle() << " err " << GetErrorInfo().GetErrcode();
      break;
    case CcErrc::unknown_proto_message_from_server:
      ss << GetErrcTitle() << " msgcode " << GetErrorInfo().GetMsgCode();
      break;
    case CcErrc::proto_read_error:
      ss << GetErrcTitle() << " pt_err = " << GetErrorInfo().GetProtoError().MakeErrorMessage();
      break;
    case CcErrc::command_while_command:
      ss << GetErrcTitle() << " (new_cmd: " << GetErrorInfo().GetStringVal() << ")";
      break;
    case CcErrc::seqcmd_aborted:
      ss << GetErrcTitle();
      break;
    case CcErrc::hshake_aborted:
      ss << GetErrcTitle();
      break;
    case CcErrc::hshake_reply_expected:
      ss << GetErrcTitle();
      break;
    case CcErrc::write_failed:
      ss << GetErrcTitle() << " " << GetErrorInfo().GetErrcode();
      break;
    case CcErrc::transport_error_hwcr:
      ss << GetErrcTitle() << " " << GetErrorInfo().GetErrcode();
      break;
    default: return DefaultErrcTitleUnknown();
    }
    return ss.str();
  }
};

// ---

using HandlerWithCcErr = Func<void(const CcError&)>;

}


