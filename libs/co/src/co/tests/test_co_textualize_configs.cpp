#include "co/async/configs/thread_model_config_from_dict.h"
#include "co/async/configs/thread_model_config_to_dict.h"
#include "co/xlog/configs/log_config_from_dict.h"

#include "co/xlog/xlog.h"
#include "co/base/cmdline/keyed_cmdline.h"
#include "co/base/tests.h"

using namespace std;
using namespace co;
using namespace co::cmdline;
using namespace co::async;
using namespace co::async::configs;
using namespace co::xlog::configs;

void test_co_textualize_configs(TestInfo& ti) {

  // check with old threadmodel
  ThreadModelConfigFromDict tmconf;

  StringMap tmdict;
  ThreadModelConfigToDict(tmdict, tmconf);

  for (const auto& field : tmdict) {
    stringstream ss;
    syslog(_INFO) << field.first << " => " << field.second << "\n";
  }
  syslog(_INFO) << "\n";

  KeyedCmdLine<char> kcl("xx.exe", tmdict, {});
  string buf;
  kcl.Textualize(buf);
  syslog(_INFO) << buf << "\n";

  KeyedCmdLine<char> kcl2(buf.c_str());
  ThreadModelConfigFromDict tmconf2(ThreadModelConfig(), kcl2.GetNamedArgs());

  //DCHECK(tmconf == tmconf2); // can't do((
  DCHECK(tmconf.force_this_thread == tmconf2.force_this_thread);
  DCHECK(tmconf.num_threads_default == tmconf2.num_threads_default);

}




