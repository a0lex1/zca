#include "co/async/test_kit/dbserver_stress_test.h"
#include "co/async/configs/thread_model_config_from_dict.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;
using namespace co::async::configs;
using namespace co::async::test_kit;
using time_duration = boost::posix_time::time_duration;

void stresstest_co_async_dbserver_u(TestInfo& ti) {

  ThreadModelConfigFromDict tm_conf(ThreadModelConfig(), ti.opts_dict, ConsumeAction::kDontConsume);

  DbserverTestParamsFromDict user_par(DbserverTestParams(), ti.opts_dict, ConsumeAction::kDontConsume);

  syslog(_INFO) << "# USER-selected params:\n";

  stringstream ss;
  user_par.Print(ss, 4);
  syslog(_INFO) << ss.str() << "\n";

  //DbserverStressTest stest(tm_conf, user_par);
  ThreadModel tm(tm_conf);
  DbserverStressTest stest(tm, user_par);

  stest.DoStressTest();

  syslog(_INFO) << "Done testing with USER params\n";
}


namespace {
static void DoTestWithConf(const ThreadModelConfig& tm_conf, const DbserverTestParams& parm) {
  ThreadModel tm(tm_conf);
  DbserverStressTest stest(tm, parm);
  syslog(_INFO) << "\nTesting with HARDCODED params => " << parm.PrintOneLine() << "; threads="<<tm_conf.num_threads_default<<"\n";
  stest.DoStressTest();
  syslog(_INFO) << "Done test with HARDCODED params.\n\n";
}

static void DoFcknTest(StringMap& opts_dict, const DbserverTestParams& parm) {
  syslog(_INFO) << "BEGIN Testing on different thread models, with HARDCODED params => " << parm.PrintOneLine() << "\n";
  
  //DoTestWithConf(ThreadModelConfig(true, { 0 }), parm); // can't do, tm won't allow
  //DoTestWithConf(ThreadModelConfig(true, { 1 }), parm); // can't do, tm won't allow
  //DoTestWithConf(ThreadModelConfig(true, { }), parm); // can't do, tm won't allow
  //DoTestWithConf(ThreadModelConfig(true, { }, 0), parm); // can't do, test will fail (server_stopped will be false)
  //DoTestWithConf(ThreadModelConfig(false, { 0 }), parm); // can't do, test will fail (server_stopped will be false)
  DoTestWithConf(ThreadModelConfig(false, { }, 1), parm);
  DoTestWithConf(ThreadModelConfig(false, { }, 2), parm);
  DoTestWithConf(ThreadModelConfig(false, { }, 3), parm);
  DoTestWithConf(ThreadModelConfig(false, { }, 7), parm);
  DoTestWithConf(ThreadModelConfig(false, { }, 13), parm);
  DoTestWithConf(ThreadModelConfig(false, { }, 24), parm);

  syslog(_INFO) << "DONE test on a different thread models, with HARDCODED params.\n";
}

static DbserverTestParams MakeDbTestParams(size_t num_conflood,
                                           size_t num_writer,
                                           size_t write_count_multi,
                                           size_t write_size_multi,
                                           size_t write_delay_multi,
                                           size_t stop_delay_ms,
                                           bool stop_ioc_instead,
                                           size_t& yyy_counter) {
  DbserverTestParams pars;
  pars.Hint.iteration_number = yyy_counter;
  pars.num_conflood_sessions = num_conflood;
  pars.num_writer_sessions = num_writer;
  pars.write_count_multipl = write_count_multi;
  pars.write_size_multipl = write_size_multi;
  pars.write_delay_multipl = write_delay_multi;
  pars.server_stop_delay_ms = stop_delay_ms;
  pars.stop_ioc_instead_of_server = stop_ioc_instead;
  yyy_counter += 1;
  return pars;
}
} // namespace {

// ----------------------------------------------------------------------------------------------------------------------------------------

/*
size_t num_conflood,
size_t num_writer,

size_t write_count_multi,
size_t write_size_multi,
size_t write_delay_multi,

size_t stop_delay_ms,
bool stop_ioc_instead)
*/

void stresstest_co_async_dbserver_easy(TestInfo& ti) {

  size_t hardness = 0;
  OverrideFromDict<string, string, size_t>(ti.opts_dict, "hardness", hardness, ConsumeAction::kDontConsume);

  size_t b = 0;
  StringMap& d(ti.opts_dict);

  // Gordon Freeman is visiting each reactor's block. The first one is finna be A.
  // *** BLOCK A ***
  // params - our tester must support these
  DoFcknTest(d, MakeDbTestParams(0, 0, 0, 0, 0, 0/*stop-delay*/, false, b));
  DoFcknTest(d, MakeDbTestParams(0, 0, 0, 0, 0, 0/*stop-delay*/, true, b));
  DoFcknTest(d, MakeDbTestParams(0, 0, 1, 2, 3, 10/*stop-delay*/, true, b));
  DoFcknTest(d, MakeDbTestParams(0, 0, 1, 2, 3, 10/*stop-delay*/, true, b));

  // *** BLOCK B ***
  // 1-0-11 
  DoFcknTest(d, MakeDbTestParams(1, 0, 0, 0, 0, 0/*stop-delay*/, false, b));
  DoFcknTest(d, MakeDbTestParams(0, 1, 0, 0, 0, 0/*stop-delay*/, false, b));
  DoFcknTest(d, MakeDbTestParams(1, 1, 0, 0, 0, 0/*stop-delay*/, false, b));
  // 1-0-11 , stop_ioc -> true
  DoFcknTest(d, MakeDbTestParams(1, 0, 0, 0, 0, 0/*stop-delay*/, true, b));
  DoFcknTest(d, MakeDbTestParams(0, 1, 0, 0, 0, 0/*stop-delay*/, true, b)); //
  DoFcknTest(d, MakeDbTestParams(1, 1, 0, 0, 0, 0/*stop-delay*/, true, b));

  // *** BLOCK B with stop-delay=30 ***
  // 1-0-11 , delay -> 30
  DoFcknTest(d, MakeDbTestParams(1, 0, 0, 0, 0, 30/*stop-delay*/, false, b));
  DoFcknTest(d, MakeDbTestParams(0, 1, 0, 0, 0, 30/*stop-delay*/, false, b));
  DoFcknTest(d, MakeDbTestParams(1, 1, 0, 0, 0, 30/*stop-delay*/, false, b));
  // 1-0-11 , stop_ioc -> true, delay -> 30
  DoFcknTest(d, MakeDbTestParams(1, 0, 0, 0, 0, 30/*stop-delay*/, true, b));
  DoFcknTest(d, MakeDbTestParams(0, 1, 0, 0, 0, 30/*stop-delay*/, true, b));
  DoFcknTest(d, MakeDbTestParams(1, 1, 0, 0, 0, 30/*stop-delay*/, true, b));

  // *** BLOCK C ***
  // 11-11 with delayed write, testing stop delays, try interfere 1-packet writer delay and server stop delay
  DoFcknTest(d, MakeDbTestParams(11, 11, 1, 1, 5, 0/*stop-delay*/, false, b));
  DoFcknTest(d, MakeDbTestParams(11, 11, 1, 1, 5, 1/*stop-delay*/, false, b));
  DoFcknTest(d, MakeDbTestParams(11, 11, 1, 1, 5, 3/*stop-delay*/, false, b));
  DoFcknTest(d, MakeDbTestParams(11, 11, 1, 1, 5, 5/*stop-delay*/, false, b));
  DoFcknTest(d, MakeDbTestParams(11, 11, 1, 1, 5, 7/*stop-delay*/, false, b));
  DoFcknTest(d, MakeDbTestParams(11, 11, 1, 1, 5, 21/*stop-delay*/, false, b));
  // longer
  DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 0/*stop-delay*/, false, b));
  DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 1/*stop-delay*/, false, b));
  DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 7/*stop-delay*/, false, b));
  DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 21/*stop-delay*/, false, b));
  if (hardness > 0) {
    DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 45/*stop-delay*/, false, b));
    DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 50/*stop-delay*/, false, b));
    DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 55/*stop-delay*/, false, b));
    DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 73/*stop-delay*/, false, b));
    DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 101/*stop-delay*/, false, b));
    // WITH stop_ioc=true ; 11-11 with delayed write, testing stop delays, try interfere 1-packet writer delay and server stop delay
    DoFcknTest(d, MakeDbTestParams(11, 11, 1, 1, 5, 0/*stop-delay*/, true, b));
    DoFcknTest(d, MakeDbTestParams(11, 11, 1, 1, 5, 1/*stop-delay*/, true, b)); //<C-3-2
    DoFcknTest(d, MakeDbTestParams(11, 11, 1, 1, 5, 3/*stop-delay*/, true, b));
    DoFcknTest(d, MakeDbTestParams(11, 11, 1, 1, 5, 5/*stop-delay*/, true, b));
    DoFcknTest(d, MakeDbTestParams(11, 11, 1, 1, 5, 7/*stop-delay*/, true, b));
    DoFcknTest(d, MakeDbTestParams(11, 11, 1, 1, 5, 21/*stop-delay*/, true, b));
    // WITH stop_ioc=true ; longer
    DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 0/*stop-delay*/, true, b));
    DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 1/*stop-delay*/, true, b));
    DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 7/*stop-delay*/, true, b));
    DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 21/*stop-delay*/, true, b));
    DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 45/*stop-delay*/, true, b));
    DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 50/*stop-delay*/, true, b));
    DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 55/*stop-delay*/, true, b));
    DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 73/*stop-delay*/, true, b));
    DoFcknTest(d, MakeDbTestParams(17, 13, 1, 1, 11, 101/*stop-delay*/, true, b));
  }

  // extra : add more write data (babe, how old a u? - 16. - 18?. -16. -18?)
  // with stop-delay=125
  DoFcknTest(d, MakeDbTestParams(53, 53, 16, 18, 50, 125/*stop-delay*/, false, b));
  DoFcknTest(d, MakeDbTestParams(53, 53, 33, 37, 50, 125/*stop-delay*/, false, b));
  // with stop-delay=125 and stop-ioc=true
  DoFcknTest(d, MakeDbTestParams(53, 53, 16, 18, 50, 125/*stop-delay*/, true, b));
  DoFcknTest(d, MakeDbTestParams(53, 53, 33, 37, 50, 125/*stop-delay*/, true, b));

  // Gordon Freeman died. Subject: Freeman. Death cause: BORED
}






