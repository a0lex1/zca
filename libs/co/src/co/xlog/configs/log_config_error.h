#pragma once

#include "co/base/error.h"
#include "co/common.h"

namespace co {
namespace xlog {
namespace configs {

enum class LogConfigErrc {
  ok = 0,
  module_not_found_for_severity = 20,
  need_only_one_asterisk = 40,
  //asterisk_not_present = 60, // * not required anymore
  need_only_one_syslog = 80,
  cannot_open_output_file = 100
};

struct LogConfigErrorInfo {
  std::string string1;

  LogConfigErrorInfo() {}
  LogConfigErrorInfo(const std::string& _string1) : string1(_string1) {}
};

class LogConfigError : public Error<LogConfigErrc, LogConfigErrorInfo> {
public:
  virtual ~LogConfigError() = default;

  using Error::Error;

  static LogConfigError NoError() { return LogConfigError(); }

  const char* GetErrcTitle() const {
    switch (GetErrc()) {
    case LogConfigErrc::ok: return DefaultErrcTitleOk();
    case LogConfigErrc::module_not_found_for_severity: return "module not found";
    case LogConfigErrc::need_only_one_asterisk: return "\"*\" specified more than once";
    //case LogConfigErrc::asterisk_not_present: return "\"*\" not specified";
    case LogConfigErrc::cannot_open_output_file: return "can't open log file";
    default: return DefaultErrcTitleUnknown();
    }
  }
  std::string MakeErrorMessage() const {
    switch (GetErrc()) {
    case LogConfigErrc::ok: return DefaultErrcTitleOk();
    case LogConfigErrc::module_not_found_for_severity: return GetErrcTitle() + std::string(": module '") + GetErrorInfo().string1 + "'";
    case LogConfigErrc::need_only_one_asterisk: return GetErrcTitle();
    //case LogConfigErrc::asterisk_not_present: return GetErrcTitle();
    case LogConfigErrc::cannot_open_output_file: return GetErrcTitle() + std::string(" - ") + GetErrorInfo().string1;
    default: return DefaultErrcTitleUnknown();
    }
  }
};


}}}

