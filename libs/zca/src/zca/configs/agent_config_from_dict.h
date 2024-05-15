#pragma once

#include "zca/agent_config.h"

#include "co/net/default_endpoint_scheme_registry.h"
#include "co/base/config.h"
#include "co/common.h"

class AgentConfigFromDictNoexcept
  :
  public co::ConfigFromDictNoexcept<AgentConfig, std::string, std::string> {
public:
  virtual ~AgentConfigFromDictNoexcept() = default;

  using ConfigType = AgentConfig;
  using ConfigError = co::ConfigError;
  using ConfigErrc = co::ConfigErrc;
  using ConfigErrorInfo = co::ConfigErrorInfo;

  AgentConfigFromDictNoexcept() = default;
  AgentConfigFromDictNoexcept(const AgentConfig& default_config,
                            StringMap& dict,
                            co::ConsumeAction consume_action, const std::vector<std::string>& required_fields = {})
    :
    ConfigFromDictNoexcept(default_config, dict, consume_action, required_fields)
  {
    Parse();
  }
private:
  void Parse() {
    std::string back_uri;
    if (!OverrideFromConfigField<std::string>("back-rem-uri", back_uri)) {
      return;
    }
    // If value (string) present, create endpoint from it
    if (NumConsumed() != 0) {
      auto& registry = co::net::GetDefaultEpSchemeRegistry();
      Errcode err;
      registry.CreateEndpointForURI(back_uri, back_remaddr, err);
      if (err) {
        SetError(ConfigError(ConfigErrc::internal_parse_failed, ConfigErrorInfo(back_uri, "")));
        return;
      }
    }
  }
private:
};

// ------------- with exception

class AgentConfigFromDict : public co::ConfigFromDict<AgentConfigFromDictNoexcept, std::string, std::string> {
public:
  using ConfigFromDict::ConfigFromDict;

  virtual ~AgentConfigFromDict() = default;
};

// ---------------------------------------------------------------------------------------------------------------------

class AgentSeparationConfigFromDictNoexcept
  :
  public co::ConfigFromDictNoexcept<AgentSeparationConfig, std::string, std::string> {
public:
  virtual ~AgentSeparationConfigFromDictNoexcept() = default;

  using ConfigType = AgentSeparationConfig;

  AgentSeparationConfigFromDictNoexcept() {}
  AgentSeparationConfigFromDictNoexcept(const AgentSeparationConfig& default_config,
    StringMap& dict,
    co::ConsumeAction consume_action, const std::vector<std::string>& required_fields = {})
    :
    ConfigFromDictNoexcept(default_config, dict, consume_action, required_fields)
  {
    Parse();
  }
private:
  void Parse() {
    // TODO
  }
private:
};

// ------------- with exception

class AgentSeparationConfigFromDict : public co::ConfigFromDict<AgentSeparationConfigFromDictNoexcept, std::string, std::string> {
public:
  using ConfigFromDict::ConfigFromDict;

  virtual ~AgentSeparationConfigFromDict() = default;
};
