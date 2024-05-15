#include "co/async/test_kit/dbserver_stress_test.h"
#include "co/async/session_park.h"
#include "co/async/server.h"
#include "co/async/tcp.h"
#include "co/async/test_kit.h"
#include "co/async/configs/thread_model_config_from_dict.h"
#include "co/base/tests.h"
#include "co/base/dict.h"
#include "co/xlog/xlog.h"
#include <boost/coroutine/all.hpp>

using namespace std;
using namespace co::async::test_kit;
using namespace co;
using namespace co::async;
using namespace co::async::configs;
using boost::posix_time::time_duration;
using boost::posix_time::milliseconds;
using namespace boost::posix_time;

typedef boost::coroutines::asymmetric_coroutine< DbserverTestParams& >::pull_type pull_coro_t;
typedef boost::coroutines::asymmetric_coroutine< DbserverTestParams& >::push_type push_coro_t;

// ------------------------------------------------------------------------------------------------------------------------

static void dbserver_test_params_generator_func(push_coro_t& sink,
                                                size_t single_iter,
                                                size_t count) {
  DbserverTestParams par;
  size_t cur_iter = 0;

  for (size_t rep = 0; rep < count; rep++) {

    for (auto& ncs : { 0, 5, 173 }) {
      par.num_conflood_sessions = ncs;
      for (auto& nws : { 1, 7, 51 }) {
        par.num_writer_sessions = nws;
        for (auto& wcm : { 3 }) {
          par.write_count_multipl = wcm;
          for (auto& wsm : { 1 }) {
            par.write_size_multipl = wsm;
            for (par.write_delay_multipl = 0; par.write_delay_multipl < 8; par.write_delay_multipl += 2) {

              for (auto& ssd : { 0, 5, 37, 228, 877 }) {

                list<bool> stopioc{ true, false };
                for (auto& sioc : stopioc) {
                  par.stop_ioc_instead_of_server = sioc;
                  par.server_stop_delay_ms = ssd;
                  if (single_iter == -1 || (cur_iter == single_iter)) {

                    par.Hint.iteration_number = cur_iter;
                    sink(par);
                  }
                  cur_iter++;
                }
              }
            }
          }
        }
      }
    }
  }
}

// ------------------------------------------------------------------------------------------------------------------------

void stresstest_co_async_dbserver_hard(TestInfo& info) {

  size_t limit = -1;
  OverrideFromDict<string, string, size_t>(info.opts_dict, "stress-limit", limit, ConsumeAction::kDontConsume);

  size_t _kRepeatCount = 1;
  ThreadModelConfigFromDict tm_conf(ThreadModelConfig(), info.opts_dict, ConsumeAction::kDontConsume);
  pull_coro_t genparams(co::bind(&dbserver_test_params_generator_func,
                        _1, -1, /* single_iter  */ _kRepeatCount)
  );
  while (genparams) {
    if (limit != -1) {
      if (limit == 0) {
        syslog(_INFO) << "Limit reached\n";
        break;
      }
      limit -= 1;
    }
    DbserverTestParams par(genparams.get());
    syslog(_INFO) << par.PrintOneLine() << "\n";

    syslog(_INFO) << " --- or in struct view ---\n";
    par.Print(cout, 8);

    ThreadModel tm(tm_conf);

    //DbserverStressTest stest(tm_conf, par);
    DbserverStressTest stest(tm, par);
    stest.DoStressTest();

    cout << "\n"; // 


    genparams();
  }

}



