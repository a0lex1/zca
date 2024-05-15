#pragma once

#include "co/base/config.h"
#include "co/xlog/configs/log_config.h"
#include <map>
#include <vector>

namespace co {

namespace xlog {
namespace configs {

class LogConfigFromDictNoexcept: public ConfigFromDictNoexcept<LogConfig, std::string, std::string> {
public:
  virtual ~LogConfigFromDictNoexcept() = default;

  using ConfigType = LogConfig;
  using string = std::string;

  LogConfigFromDictNoexcept() {}
  LogConfigFromDictNoexcept(const LogConfig& default_config,
    StringMap& dict, ConsumeAction consume_action,
    const std::vector<string>& required_fields = {});

private:
#ifndef CO_XLOG_DISABLE
  void Parse();
#endif

private:
};

// ------------- with exception

class LogConfigFromDict : public ConfigFromDict<LogConfigFromDictNoexcept, std::string, std::string> {
public:
  using ConfigFromDict::ConfigFromDict;

  virtual ~LogConfigFromDict() = default;
};

}}

}

