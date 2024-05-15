#pragma once

#include "zca/backend_config.h"

#include "co/net/default_endpoint_scheme_registry.h"
#include "co/net/parse_scheme_address.h"
#include "co/base/config.h"
#include "co/common.h"

class BackendConfigFromDictNoexcept
  :
  public co::ConfigFromDictNoexcept<BackendConfig, std::string, std::string> {
public:
  virtual ~BackendConfigFromDictNoexcept() = default;

  using ConfigType = BackendConfig;
  using ConfigError = co::ConfigError;
  using ConfigErrc = co::ConfigErrc;
  using ConfigErrorInfo = co::ConfigErrorInfo;

  BackendConfigFromDictNoexcept() {}
  BackendConfigFromDictNoexcept(const BackendConfig& default_config,
    StringMap& dict,
    co::ConsumeAction consume_action, const std::vector<std::string>& required_fields = {})
    :
    ConfigFromDictNoexcept(default_config, dict, consume_action, required_fields)
  {
    Parse();
  }
private:
  void Parse() {
    std::string admin_server_uri;
    if (!OverrideFromConfigField<std::string>("admin-loc-uri", admin_server_uri)) {
      return;
    }
    // If value (string) present, create endpoint from it
    if (NumConsumed() != 0) {
      auto& registry = co::net::GetDefaultEpSchemeRegistry();
      Errcode err;
      registry.CreateEndpointForURI(admin_server_uri, admin_server_locaddr, err);
      if (err) {
        SetError(ConfigError(ConfigErrc::internal_parse_failed, ConfigErrorInfo(admin_server_uri, "")));
        return;
      }
    }

    // TODO: cc-locuri
    // --cc-locuri tcp://0.0.0.0:0
  }
private:
};

// ------------- with exception

class BackendConfigFromDict : public co::ConfigFromDict<BackendConfigFromDictNoexcept, std::string, std::string> {
public:
  using ConfigFromDict::ConfigFromDict;

  virtual ~BackendConfigFromDict() = default;
};

// ---------------------------------------------------------------------------------------------------------

class BackendSeparationConfigFromDictNoexcept
  :
  public co::ConfigFromDictNoexcept<BackendSeparationConfig, std::string, std::string> {
public:
  virtual ~BackendSeparationConfigFromDictNoexcept() = default;

  using ConfigType = BackendSeparationConfig;

  BackendSeparationConfigFromDictNoexcept() {}
  BackendSeparationConfigFromDictNoexcept(const BackendSeparationConfig& default_config,
                                          StringMap& dict,
                                          co::ConsumeAction consume_action, const std::vector<std::string>& required_fields = {})
    :
    ConfigFromDictNoexcept(default_config, dict, consume_action, required_fields)
  {
    Parse();
  }
private:
  void Parse() {
    if (!OverrideFromConfigField<size_t>("admin-acpt-thread-group", admin_acceptor_threadgroup)) {
      return;
    }
    if (!OverrideFromConfigField<size_t>("admin-sessions-thread-group", admin_sessions_threadgroup)) {
      return;
    }
  }
private:
};

// ------------- with exception

class BackendSeparationConfigFromDict : public co::ConfigFromDict<BackendSeparationConfigFromDictNoexcept, std::string, std::string> {
public:
  using ConfigFromDict::ConfigFromDict;

  virtual ~BackendSeparationConfigFromDict() = default;
};


