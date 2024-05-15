#pragma once

#include "co/base/config.h"
#include "co/async/thread_model.h"
#include <map>
#include <vector>

namespace co {
namespace async {
namespace configs {

class ThreadModelConfigFromDictNoexcept: public ConfigFromDictNoexcept<ThreadModelConfig, std::string, std::string> {
public:
  virtual ~ThreadModelConfigFromDictNoexcept() = default;

  using ConfigType = ThreadModelConfig;

  ThreadModelConfigFromDictNoexcept() {}
  ThreadModelConfigFromDictNoexcept(const ThreadModelConfig& default_config,
    StringMap& dict,
    ConsumeAction consume_action,
    const std::vector<std::string>& required_fields = {});

private:
  void Parse();
};

// ------------- with exception

class ThreadModelConfigFromDict : public ConfigFromDict<ThreadModelConfigFromDictNoexcept, std::string, std::string> {
public:
  using ConfigFromDict::ConfigFromDict;

  virtual ~ThreadModelConfigFromDict() = default;
};

}}}

