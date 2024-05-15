#include "co/async/configs/thread_model_config_from_dict.h"

using namespace std;

namespace co {
namespace async {
namespace configs {

ThreadModelConfigFromDictNoexcept::ThreadModelConfigFromDictNoexcept(
  const ThreadModelConfig& default_config, StringMap& dict,
  ConsumeAction consume_action, const std::vector<string>& required_fields /*= {}*/)
  :
  ConfigFromDictNoexcept(default_config, dict, consume_action, required_fields)
{
  Parse();
}

void ThreadModelConfigFromDictNoexcept::Parse()
{
  if (!OverrideFromConfigField<bool>("this-thread", force_this_thread)) {
    return;
  }
  if (!OverrideFromConfigField<size_t>("num-threads-default", num_threads_default)) {
    return;
  }
  // TODO:
  //     for group : num-threads-for-group
  //
  if (force_this_thread) {
    // Can't be combined with --this-thread
    if (!num_threads_for_group.empty()) {
      SetError(ConfigError(ConfigErrc::conflicting_fields,
        ConfigErrorInfo("this-thread", "num-threads-for-group")));
      return;
    }
    if (num_threads_default != 0) {
      SetError(ConfigError(ConfigErrc::conflicting_fields,
        ConfigErrorInfo("this-thread", "num-threads-for-group")));
      return;
    }
  }
}

}}}

