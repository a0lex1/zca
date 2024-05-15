#pragma once

#include "co/base/dict.h"
#include "co/base/error_exception.h"

namespace co {

enum class ConfigErrc {
  ok = 0,
  dict_error = 10,
  conflicting_fields = 20,
  internal_parse_failed = 30,
  internal_no_object = 40,
  //internal_object_failed = 50,

};

struct ConfigErrorInfo {
  DictError dict_error; // if errc == dict_error
  std::string string1; // if conflicting_fields, field1 and field2
  std::string string2;

  ConfigErrorInfo() {}
  ConfigErrorInfo(const DictError& _dict_error) : dict_error(_dict_error) {}
  ConfigErrorInfo(const std::string& str1, const std::string& str2) : string1(str1), string2(str2) {}
};

class ConfigError : public Error<ConfigErrc, ConfigErrorInfo> {
public:
  virtual ~ConfigError() = default;

  using Error::Error;

  ConfigError() {}
  ConfigError(const DictError& dict_error) : ConfigError(ConfigErrc::dict_error, ConfigErrorInfo(dict_error)) {}

  static ConfigError NoError() { return ConfigError(); }

  const char* GetErrcTitle() const {
    switch (GetErrc()) {
    case ConfigErrc::ok: return DefaultErrcTitleOk();
    case ConfigErrc::dict_error: return "key-value dict error";
    case ConfigErrc::conflicting_fields: return "conflicting fields";
    default: return DefaultErrcTitleUnknown();
    }
  }
  std::string MakeErrorMessage() const {
    switch (GetErrc()) {
    case ConfigErrc::dict_error:
      return GetErrcTitle() + std::string(": ") + GetErrorInfo().dict_error.MakeErrorMessage();
    case ConfigErrc::conflicting_fields:
      return GetErrcTitle() + std::string(": ") + GetErrorInfo().string1 + " and " + GetErrorInfo().string2;
    default:
      return GetErrcTitle() + std::string(":");
    }
  }
};

class ConfigException : public ErrorException<ConfigError> {
  using ErrorException::ErrorException;
};

// ----------------------------------------------------------------------------------------------------

// WITHOUT EXCEPTION

template <typename Config, typename K, typename V>
class ConfigFromDictNoexcept : public Config {
public:
  virtual ~ConfigFromDictNoexcept() = default;

  ConfigFromDictNoexcept()
  {
  }

  ConfigFromDictNoexcept(const Config& default_config, std::map<K, V>& dict,
                         ConsumeAction consume_action = ConsumeAction::kDontConsume,
                         const std::vector<K>& required_fields = {})
    :
    Config(default_config)
  {
    PrepareToParse(dict, consume_action, required_fields);
  }

  void PrepareToParse(std::map<K, V>& dict,
                      ConsumeAction consume_action = ConsumeAction::kDontConsume,
                      const std::vector<K>& required_fields = {})
  {
    dict_ = &dict;
    consume_action_ = consume_action;
    required_fields_ = &required_fields;
    num_consumed_ = 0;
  }

  const ConfigError& GetError() const { return error_; }
  size_t NumConsumed() const { return num_consumed_; }
  ConsumeAction GetConsumeAction() const { return consume_action_; }

protected:
  void IncreaseNumConsumed(size_t add) { num_consumed_ += add; }

  // For use in derived classes. Saves |error_|. So derived classes
  // should just return if error occurred.
  //
  // --obsolete--  For users of this class to know whether the field is default-skipped (optional)
  // --obsolete--  or successfully-parsed, we introduce |raw_value| argument.
  //
  // Increases |num_consumed_|. Use it to know if the field is default-skipped (optional)
  // or successfully-parsed
  //

  template <typename CV>
  bool OverrideFromConfigField(const K& key, CV& val) {
    error_ = OverrideFromDictNoexceptWithReqF<CV>(
      *dict_, key, val, consume_action_, *required_fields_,
      num_consumed_
      );
    return !error_;
  }
  void SetError(const ConfigError& error) {
    error_ = error;
  }
  // Use cases:
  // dbserver_test_params.cpp!DbserverTestParamsFromDictNoExcept::Parse(): GetDict, GetRequiredFields
  std::map<K, V>& GetDict() { return *dict_; }
  const std::vector<K>& GetRequiredFields() { return *required_fields_; }

private:

  // wrapper around dict that supports |required_fields|
  template <typename CV>
  static ConfigError OverrideFromDictNoexceptWithReqF(
    std::map<K, V>& dict,
    const K& key,
    CV& value,
    ConsumeAction consume_action,
    const std::vector<std::string>& required_fields,
    size_t& num_consumed)
  {
    bool is_required = find(required_fields.begin(), required_fields.end(), key) != required_fields.end();
    bool was_found;
    DictError dict_err = OverrideFromDictNoexcept(
      dict, key, value,
      consume_action,
      is_required ? Necessity::kRequired : Necessity::kOptional,
      &was_found
    );
    if (dict_err) {
      return ConfigError(ConfigErrc::dict_error, dict_err);
    }
    if (was_found) {
      if (consume_action == ConsumeAction::kConsume) {
        // Was found and was consumed, increase |num_consumed_|
        num_consumed += 1;
      }
    }
    return ConfigError::NoError();
  }

private:
  // pointers* for copyability for operator =
  std::map<K, V>* dict_{ nullptr };
  ConsumeAction consume_action_{ ConsumeAction::kDontConsume };
  const std::vector<K>* required_fields_{ nullptr };
  ConfigError error_;
  size_t num_consumed_{ 0 };
};

// WITH EXCEPTION

template <class NoexceptBase, typename K, typename V>
class ConfigFromDict : public NoexceptBase {
public:
  virtual ~ConfigFromDict() = default;

  using ConfigType = typename NoexceptBase::ConfigType;

  ConfigFromDict() { }

  ConfigFromDict(const ConfigType& default_config,
    std::map<K, V>& dict,
    ConsumeAction consume_action = ConsumeAction::kDontConsume,
    const std::vector<K>& required_fields = {})
    :
    NoexceptBase(default_config, dict, consume_action, required_fields)
  {
    if (NoexceptBase::GetError()) {
      BOOST_THROW_EXCEPTION(ConfigException(NoexceptBase::GetError()));
    }
  }
};


}

