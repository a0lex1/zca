#include "co/async/configs/thread_model_config_to_dict.h"
#include "co/base/strings.h"

namespace co {
namespace async {
namespace configs {

void ThreadModelConfigToDict(StringMap& sm, const ThreadModelConfig& tmc) {
  sm.clear();
  sm["this-thread"] = tmc.force_this_thread ? "1" : "0";
  sm["num-threads-default"] = string_printf("%d", tmc.num_threads_default);

}


}}}
