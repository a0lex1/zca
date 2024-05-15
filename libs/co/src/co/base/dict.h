#pragma once

#include "co/base/convert_value.h"
#include "co/base/error_exception.h"
#include "co/common.h"
#include <map>

// ----------------------------------------------------------------------------------------------------

namespace co {

enum class DictErrc {
  ok = 0,
  required_opt_not_found = 10,
  opt_parsing_failed = 20
};

struct DictErrorInfo {
  std::string problem_key;
  std::string problem_value;

  DictErrorInfo() {}
  DictErrorInfo(const std::string& _problem_key, const std::string& _problem_value) : problem_key(_problem_key), problem_value(_problem_value) {}
};

class DictError : public Error<DictErrc, DictErrorInfo> {
public:
  virtual ~DictError() = default;

  using Error::Error;

  static DictError NoError() { return DictError(); }

  const char* GetErrcTitle() const {
    switch (GetErrc()) {
    case DictErrc::ok: return DefaultErrcTitleOk();
    case DictErrc::required_opt_not_found: return "required opt not found";
    case DictErrc::opt_parsing_failed: return "opt parsing error";
    default: return DefaultErrcTitleUnknown();
    }
  }
  std::string MakeErrorMessage() const {
    switch (GetErrc()) {
    case DictErrc::ok: return DefaultErrcTitleOk();
    case DictErrc::required_opt_not_found: return GetErrcTitle() + std::string(" - ") + GetErrorInfo().problem_key;
    case DictErrc::opt_parsing_failed: return GetErrcTitle() + std::string(": key '") + GetErrorInfo().problem_key + "', value: '" + GetErrorInfo().problem_value + "'";
    default: return DefaultErrcTitleUnknown();
    }
  }
};

class DictException : public ErrorException<DictError> {
  using ErrorException::ErrorException;
};

// ----------------------------------------------------------------------------------------------------

enum class ConsumeAction {
  kConsume,
  kDontConsume
};

enum class Necessity {
  kOptional,
  kRequired
};

// Function OverrideFromDictNoexcept() : utility helper for many cases:
//   You can use it directly in your code anywhere
//   It is used by ConfigFromDict<>

template <typename K, typename V, typename CV>
static DictError OverrideFromDictNoexcept(
  std::map<K, V>& dict,
  const K& key,
  CV& value,
  ConsumeAction consume_action,
  Necessity optional_or_required = Necessity::kOptional,
  bool* was_found = nullptr)
{
  auto it = dict.find(key);
  if (it == dict.end()) {
    // key not found
    if (optional_or_required == Necessity::kRequired) {
      // not found and key is required (not optional)
      return DictError(DictErrc::required_opt_not_found, DictErrorInfo(key, ""));
    }
    else {
      // not found, but key is optional, ok
      if (was_found) {
        *was_found = false;
      }
      return DictError();
    }
  }
  // found
  if (was_found) {
    *was_found = true;
  }
  if (!ConvertValue(it->second, value)) {
    return DictError(DictErrc::opt_parsing_failed, DictErrorInfo(it->first, it->second));
  }
  if (consume_action == ConsumeAction::kConsume) {
    dict.erase(it);
  }
  return DictError();
}

template <typename K, typename V, typename CV>
static void OverrideFromDict(
  std::map<K, V>& dict,
  const K& key,
  CV& value,
  ConsumeAction consume_action,
  Necessity is_required = Necessity::kOptional,
  bool* was_found = nullptr)
{
  DictError error = OverrideFromDictNoexcept(dict, key, value, consume_action, is_required, was_found);
  if (error) {
    throw DictException(error);
  }
}

// ----------------------------------------------------------------------------------------------------

// Example: ParseSingleLineDictAsAs<string, int>("a=1;b=2;") -> {{"a", 1},{"b",2}}

template <typename K = std::string, typename V = std::string>
static bool ParseSingleLineDictAs(const std::string& str, std::map<K, V>& dict) {
  dict.clear();
  StringVector parts;
  string_split(str, ";", parts);
  for (const auto& part : parts) {
    if (part.empty()) {
      continue;
    }
    StringVector key_val;
    string_split(part, ":", key_val);
    if (key_val.size() != 2) {
      return false;
    }
    K k;
    V v;
    if (!ConvertValue(key_val[0], k)) {
      return false;
    }
    if (!ConvertValue(key_val[1], v)) {
      return false;
    }
    dict[k] = v;
  }
  return true;
}

}
