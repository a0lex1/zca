#pragma once

#include "co/base/error.h"
#include "co/common.h"

namespace netshell {

enum class NetshellErrc {
  ok = 0,
  stream_error = 1, //

  // NsCmdResult::ParseFirstLine()
  bad_part_count = 2,
  bad_status_code = 3, // remote sent non-integer
  status_name_not_found = 4,
  status_name_mismatch = 5,
  bad_ret_code = 6,
  bad_result_type = 7,
  bad_body_line_count = 8,
  unknown_status_code = 9, // remote peer sent valid integer, but the code is unknown

  // InputBodyLine(), InputBodyEnd()
  unexpected_line = 10,

  // Untextualize()
  unexpected_end = 11,

  // Parallel command result reader
  cannot_parse_command_index_int = 12,

  // Parallel command executor
  invalid_op_index = 13
};

struct NetshellErrorInfo {
  Errcode stream_err_code;
  std::string bad_str;
  uint64_t index64;

  NetshellErrorInfo() {}
  NetshellErrorInfo(Errcode _stream_err_code) : stream_err_code(_stream_err_code) {}
  NetshellErrorInfo(const std::string& bad_cmdindex_int_str): bad_str(bad_cmdindex_int_str) {}
  NetshellErrorInfo(uint64_t invalid_op_index_num) : index64(invalid_op_index_num) {}
};

class NetshellError : public co::Error<NetshellErrc, NetshellErrorInfo> { // was ::co ! ))
public:
  virtual ~NetshellError() = default;

  using Error::Error;
  NetshellError() {}

  static NetshellError NoError() { return NetshellError(); }

  const char* GetErrcTitle() const {
    switch (GetErrc()) {
    case NetshellErrc::ok: return DefaultErrcTitleOk();
    case NetshellErrc::stream_error: return "stream error";
    case NetshellErrc::bad_part_count: return "bad part count";
    case NetshellErrc::bad_status_code: return "bad status code";
    case NetshellErrc::status_name_not_found: return "status name not found";
    case NetshellErrc::status_name_mismatch: return "status name mismatch";
    case NetshellErrc::bad_ret_code: return "bad ret code";
    case NetshellErrc::bad_result_type: return "bad result type";
    case NetshellErrc::bad_body_line_count: return "bad body line count";
    case NetshellErrc::unknown_status_code: return "unknown status code";
    case NetshellErrc::unexpected_line: return "unexpected line";
    case NetshellErrc::unexpected_end: return "unexpected end";
    case NetshellErrc::cannot_parse_command_index_int : return "cannot parse command index int";
    case NetshellErrc::invalid_op_index: return "invalid op index";
    default: return DefaultErrcTitleUnknown();
    }
  }
  std::string MakeErrorMessage() const;
};

}
