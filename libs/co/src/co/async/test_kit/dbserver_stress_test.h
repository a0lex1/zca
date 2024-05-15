#pragma once

#include "./dbserver_test_params.h"
#include "co/async/thread_model_config.h"

namespace co {
namespace async {
namespace test_kit {

// TO make test_co_async_capsule_listserver
// you need
// remake this class as LoopObject and use it as i/o initiator
class DbserverStressTest {
public:
  DbserverStressTest(const ThreadModelConfig& tm_conf, const DbserverTestParams& par);
  DbserverStressTest(ThreadModel& tm, const DbserverTestParams& par);

  void DoStressTest();

private:
  Uptr<ThreadModelConfig> tm_conf_;
  ThreadModel* tm_;
  const DbserverTestParams& par_;
};

}
}}


















