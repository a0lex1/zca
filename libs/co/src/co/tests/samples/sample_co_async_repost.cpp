#include "co/async/thread_model.h"
#include "co/base/tests.h"

using namespace co;
using namespace co::async;

static void RepostSelf(io_context& ioc) {
  ioc.post(co::bind(&RepostSelf, std::ref(ioc)));
}

void sample_co_async_repost(TestInfo& test_info) {

  ThreadModelConfig tmconf(false, { 24 });
  ThreadModel tm(tmconf);

  RepostSelf(tm.DefIOC());

  tm.Run();
}



