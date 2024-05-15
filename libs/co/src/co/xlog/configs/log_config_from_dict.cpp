#include "co/xlog/configs/module_severity_map.h"
#include "co/xlog/severity_from_string.h"

#include "co/common.h"

// MSVC+GCC issue. Need to define this way. Before inclusion of config/dict.
#ifndef CO_XLOG_DISABLE // no xlog in release
namespace co {
bool ConvertValue(const std::string& str, co::xlog::configs::ModuleSeverityMap& val);
}
#endif

#include "co/xlog/configs/log_config_from_dict.h"
#include "co/base/strings.h"

using namespace std;

namespace co {
namespace xlog {
namespace configs {

LogConfigFromDictNoexcept::LogConfigFromDictNoexcept(const LogConfig& default_config,
  StringMap& dict,
  ConsumeAction consume_action,
  const std::vector<string>& required_fields /*= {}*/)
  :
  ConfigFromDictNoexcept(default_config, dict, consume_action, required_fields)
{
#ifndef CO_XLOG_DISABLE
  Parse();
#endif
}

#ifndef CO_XLOG_DISABLE
void LogConfigFromDictNoexcept::Parse() {
  if (!OverrideFromConfigField<bool>("log-print-tid", do_print_tid)) {
    return;
  }
  if (!OverrideFromConfigField<bool>("log-print-func", do_print_func)) {
    return;
  }
  if (!OverrideFromConfigField<bool>("log-fix-lambdas", fix_long_lambda_names)) {
    return;
  }
  if (!OverrideFromConfigField<bool>("log-pad-func", pad_func_name)) {
    return;
  }
  if (!OverrideFromConfigField<bool>("log-fix-anon", fix_long_anonymous_names)) {
    return;
  }
  if (!OverrideFromConfigField<ModuleSeverityMap>("log-sevs", module_severities)) {
    return;
  }
  // File output
  if (!OverrideFromConfigField<string>("log-file", file_path)) {
    return;
  }
  string ring_file_path;
  if (!OverrideFromConfigField<string>("log-ring-file", ring_file_path)) {
    return;
  }
  // Can't combine --log-file and --log-ring-file, must choose one of them
  if (file_path != "" && ring_file_path != "") {
    SetError(ConfigError(ConfigErrc::conflicting_fields, ConfigErrorInfo("--log-file", "--log-ring-file")));
    return;
  }
  if (ring_file_path != "") {
    file_path = ring_file_path;
    file_ring_file = true;
  }
  //
  if (!OverrideFromConfigField<bool>("log-stdout", enable_stdout)) {
    return;
  }
  if (!OverrideFromConfigField<bool>("log-file-flush", file_do_flush)) {
    return;
  }
  if (!OverrideFromConfigField<size_t>("log-file-max", file_max_size)) {
    return;
  }
  if (!OverrideFromConfigField<bool>("log-file-append", file_append)) {
    return;
  }
}
#endif

} // namespace xlog
} // namespace configs

#ifndef CO_XLOG_DISABLE // no xlog in release
bool ConvertValue(const std::string& str, co::xlog::configs::ModuleSeverityMap& val) {
  StringMap sev_map_str;
  if (!ParseSingleLineDictAs<std::string, std::string>(str, sev_map_str)) {
    return false;
  }
  val.sev_map.clear();
  // Convert values from strings to ints (severities)
  for (auto& it : sev_map_str) {
    int sev;
    if (!co::xlog::SeverityFromString(it.second, sev)) {
      return false;
    }
    val.sev_map.insert(std::pair<std::string, int>(it.first, sev));
  }
  return true;
}
#endif

} // namespace co


