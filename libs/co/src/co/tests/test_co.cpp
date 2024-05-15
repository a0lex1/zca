#include "co/net/distance.h"
#include "co/net/subrange_enumerator.h"
#include "co/net/range_iterator.h"
#include "co/net/address_calc.h"

#include "co/base/rand_gen.h"
#include "co/base/ref_tracker.h"
#include "co/base/strings.h"
#include "co/base/cmdline/keyed_cmdline.h"
#include "co/base/tests.h"

#include "co/xlog/xlog_print.h"
#include "co/xlog/configs.h"
#include "co/xlog/xlog.h"

using namespace boost::asio::ip;
using namespace co::xlog::configs;
using namespace co::xlog;
using namespace co::net;
using namespace co::cmdline;
using namespace co;
using namespace std;

static void RefTrackerSet123ErrWorker(RefTracker rt) {
  auto rt_ = rt;
  //rt_.SetError(boost::asio::error::connection_aborted);
}

static void RefTrackerSet123Err(RefTracker rt) {
  RefTrackerSet123ErrWorker(rt);
}

static RefTracker grt1;
static RefTracker grt2;

void test_co_reftracker(TestInfo& ) {

  std::vector<std::string> call_history;
#define ADDHIST(s) call_history.push_back(#s)
  {
    {
      RefTrackerContext rtctx(CUR_LOC());

      bool called0 = false;

      RefTracker rt0(CUR_LOC(), rtctx.GetHandle(), [&]() {called0 = true;});
      rt0 = RefTracker(CUR_LOC(), rtctx.GetHandle(), []() {});
      DCHECK(called0);

      RefTracker rt_all(CUR_LOC(), rtctx.GetHandle(),
        [&]() {
          syslog(_INFO) << "rt_all called\n"; ADDHIST(rt_all);
        });

      RefTracker rt1(CUR_LOC(), [&]() { syslog(_INFO) << "rt1 called\n"; ADDHIST(rt1);}, rt_all);
      RefTracker rt2(CUR_LOC(), [&]() { syslog(_INFO) << "rt2 called\n"; ADDHIST(rt2);}, rt_all);

      RefTracker rt3n4(CUR_LOC(), [&]() { syslog(_INFO) << "rt3n4 called\n"; ADDHIST(rt3n4);}, rt_all);
      RefTracker rt3(CUR_LOC(), rtctx.GetHandle(), [&]() { syslog(_INFO) << "rt3 called\n";ADDHIST(rt3);});
      RefTracker rt4(CUR_LOC(), rtctx.GetHandle(), [&]() { syslog(_INFO) << "rt4 called\n";ADDHIST(rt4);});

      grt1 = rt1;
      RefTrackerSet123Err(rt3);
      grt2 = rt2;
    }
    grt1 = RefTracker();
    grt2 = RefTracker();
  }
  syslog(_INFO) << "CallHistory:\n";
  for (auto& c : call_history) {
    syslog(_INFO) << "  " << c << "\n";
  }
}

void test_co_reftracker_context(TestInfo& ) {
  RefTrackerContext rtctx(CUR_LOC());
  bool rt1called;

  // Without Context

  {
    RefTracker rt1(CUR_LOC(), rtctx.GetHandle(), [&]() {
      rt1called = true;
                   });
    rt1called = false;
  }
  DCHECK(rt1called);

  // With Context

  {
    RefTracker rt1(CUR_LOC(), rtctx.GetHandle(), [&]() {
      rt1called = true;
                   });
    rt1called = false;

    rtctx.DisableOnReleaseCalls();
  }
  DCHECK(!rt1called);
}

void test_co_net_distance(TestInfo& ) {
  address a = address::from_string("100.2.3.4"), b = address::from_string("6.7.8.9");

  co::net::distance d1;
  d1 = a - b;

  co::net::distance d2 = d1 + 1;
  co::net::distance d3 = d1 + d2;
  address c = a + d3;

  d2 + d2;
  d3 + d3;
  c.to_string();
}

void test_co_net_range_iterator(TestInfo& ) {
  address_range r;

  range_iterator it1, it2;
  it1 = range_begin(r);
  it2 = range_end(r);

  while (it1 != it2) {
    ++it1;
  }

  it1 = it2 - co::net::distance(5);
  it2 = it1 + co::net::distance(10);
  it1 = it1 + co::net::distance(5);
}

static void test_co_net_subrange_enumeator_on(address_range range, co::net::distance len) {
  syslog(_INFO) << "testing subrange enumerator on " << range.to_string() << " (len=" << len.v << ")\n";

  subrange_enumerator e(range, len);
  while (!e.end()) {
    syslog(_INFO)
      << " "
      << e.current_subrange().first_ip().to_string() << " - "
      << e.current_subrange().last_ip().to_string() << "\n";
    e.next();
  }
}

void test_co_net_subrange_enumerator(TestInfo& ) {

  test_co_net_subrange_enumeator_on(
    address_range(address::from_string("1.2.3.4"), address::from_string("1.2.3.12")),
    co::net::distance(5));

  test_co_net_subrange_enumeator_on(
    address_range(address::from_string("1.2.3.4"), address::from_string("1.2.3.4")),
    co::net::distance(1));

  test_co_net_subrange_enumeator_on(
    address_range(address::from_string("1.2.3.4"), address::from_string("1.2.3.4")),
    co::net::distance(2));

  test_co_net_subrange_enumeator_on(
    address_range(address::from_string("1.2.3.4"), address::from_string("1.2.3.5")),
    co::net::distance(1));

  test_co_net_subrange_enumeator_on(
    address_range(address::from_string("1.2.3.4"), address::from_string("1.2.3.5")),
    co::net::distance(2));

  test_co_net_subrange_enumeator_on(
    address_range(address::from_string("1.2.3.4"), address::from_string("1.2.17.5")),
    co::net::distance(7));
}

void test_co_net_address_calc(TestInfo& ) {
  co::net::distance x, y;
  x = address_calc::max_host(true, 8);
  y = address_calc::max_net(true, 8);
  syslog(_INFO) << x.v << ", " << y.v << "\n";
  DCHECK(x.v == 0xffffff && y.v == 0xff);

  x = address_calc::max_host(true, 9);
  y = address_calc::max_net(true, 9);
  syslog(_INFO) << x.v << ", " << y.v << "\n";
  DCHECK(x.v == 0x7FFFFF && y.v == 0x1ff);

  address a(address::from_string("18.52.86.120")); // 0x 12 34 56 78

  x = address_calc::net_num(a, 7);
  y = address_calc::host_num(a, 7);
  syslog(_INFO) << std::hex << x.v << ", " << y.v << "\n";
  DCHECK(x.v == 9 && y.v == 0x345678);

  x = address_calc::net_num(a, 27);
  y = address_calc::host_num(a, 27);
  syslog(_INFO) << std::hex << x.v << ", " << y.v << "\n";
  DCHECK(x.v == 0x91A2B3 && y.v == 0x18);

  //

  DCHECK(!address_calc::valid_network_bits(true, 0));
  DCHECK(address_calc::valid_network_bits(true, 1));
  DCHECK(address_calc::valid_network_bits(true, 24));
  DCHECK(address_calc::valid_network_bits(true, 31));
  DCHECK(!address_calc::valid_network_bits(true, 32));
  DCHECK(!address_calc::valid_network_bits(true, 33));

  DCHECK(address_calc::valid_host_num(true, 24, co::net::distance(0)));
  DCHECK(address_calc::valid_host_num(true, 24, co::net::distance(255)));
  DCHECK(!address_calc::valid_host_num(true, 24, co::net::distance(256)));
  DCHECK(address_calc::valid_host_num(true, 25, co::net::distance(127)));
  DCHECK(!address_calc::valid_host_num(true, 25, co::net::distance(128)));

  a = address_calc::make_address(true, 13, co::net::distance(77), co::net::distance(99));
  syslog(_INFO) << a << "\n";
  DCHECK(a == address::from_string("2.104.0.99"));

  DCHECK(address_calc::address_bits(true) == 32);
  DCHECK(address_calc::address_bits(false) == 128);

  DCHECK(address_calc::max_value(1).v == 1);
  DCHECK(address_calc::max_value(2).v == 3);
}


// -----------------------------

extern "C" void test_co_xlog_c99();

void test_co_xlog_c99_(TestInfo& ) {
  // c99log handles its things. It won't appear in non-_DEBUG build.
  // We can leave it outside ifdef _DDEBUG.
  test_co_xlog_c99();
}

void test_co_xlog_newlines(TestInfo&) {
  // Let's test the case where we don't give a damn about debug/release. Just use log.
  syslog(_FATAL) << "there\nare\nsome\nlines\n";
}

#ifdef CO_XLOG_DISABLE

namespace {
static void XxxtyWarn() {
  cout << "WARNING : NON-_DEBUG BUILD: SKIPPING xlog TESTS\n";
}
}
void test_co_xlog(TestInfo&) {
  XxxtyWarn();
}
void test_co_xlog_configs(TestInfo&) {
  XxxtyWarn();
}
void test_co_xlog_printsinks(TestInfo&) {
  XxxtyWarn();
}
void _test_co_xlog_ringmemdevice_with(TestInfo&) {
  XxxtyWarn();
}
void test_co_xlog_CLoggerOstreamRingDevice(TestInfo&) {
  XxxtyWarn();
}

#else // if log not disabled

namespace {
static void _test_co_xlog_with(const CLoggerOpts& opts) {
  using namespace co::xlog;
  volatile int project_zero = 0;

  Shptr<CLoggerSink> prev_syslog_sink = SetsyslogSink(
    CreateLoggerWithDevices(opts, { make_shared<co::xlog::CLoggerStdoutDevice>() }));

  syslog(_TRACE) << "this is syslog TRACE message, I give " << project_zero << " \n";
  syslog(_DBG) << "this is syslog DBG message, I give " << project_zero << " \n";
  syslog(_DBG+5) << "this is syslog DBG+5 message, I give " << project_zero << " \n";
  syslog(_INFO) << "this is syslog INFO message, I give " << project_zero << " \n";
  syslog(_ERR) << "this is syslog ERR message, I give " << project_zero << " \n";
  syslog(_FATAL+1) << "this is syslog FATAL+1 message, I give " << project_zero << " \n";
  syslog(_FATAL) << "Brain lag #" << std::hex << 0xDEADBEEF << " detected\n";

  Shptr<CLogger> logger(CreateLoggerWithDevices(opts, {make_shared<co::xlog::CLoggerStdoutDevice>()}));
#define XLOG_CURRENT_SINK logger->getSink()

  std::string test = co::string_printf("%d", 0);
  Log(_TRACE) << "this is Log TRACE message, I give " << test << " \n";
  Log(_DBG) << "this is Log DBG message, I give " << test << " \n";
  Log(_DBG+105) << "this is Log DBG+105 message, I give " << test << " \n";
  Log(_INFO) << "this is Log INFO message, I give " << test << " \n";
  Log(_ERR) << "this is Log ERR message, I give " << test << " \n";
  Log(_FATAL) << "this is systLogemlog FATAL message, I give " << test << " \n";
  Log(_FATAL) << "Pen too short (needed 0x" << std::hex << 0x6AF << " inches)\n";

  auto subsink(make_shared<CLoggerSubmoduleSink>(XLOG_CURRENT_SINK, "submod"));
#undef XLOG_CURRENT_SINK
#define XLOG_CURRENT_SINK subsink

  auto errcode(boost::asio::error::address_family_not_supported);
  Log(_TRACE) << "this is SUBMODULE Log TRACE message, error " << errcode << "\n";
  Log(_DBG) << "this is SUBMODULE Log DBG message, error " << errcode << "\n";
  Log(_INFO) << "this is SUBMODULE Log INFO message, error " << errcode << "\n";
  Log(_ERR) << "this is SUBMODULE Log ERR message, error " << errcode << "\n";
  Log(_FATAL) << "this is SUBMODULE Log FATAL message, error " << errcode << "\n";

  auto subsubsink(make_shared<CLoggerSubmoduleSink>(XLOG_CURRENT_SINK, "SUBsubmod"));
#undef XLOG_CURRENT_SINK
#define XLOG_CURRENT_SINK subsubsink

  Log(_TRACE+1337) << "this is Sub-SUBMODULE Log TRACE+1337 message, error " << errcode << "\n";
  Log(_DBG) << "this is Sub-SUBMODULE Log DBG message, error " << errcode << "\n";
  Log(_INFO) << "this is Sub-SUBMODULE Log INFO message, error " << errcode << "\n";
  Log(_ERR+1337) << "this is Sub-SUBMODULE Log ERR+1337 message, error " << errcode << "\n";
  Log(_FATAL) << "this is Sub-SUBMODULE Log FATAL message, error " << errcode << "\n";

  // C log from C++ file
  void* C_sink = static_cast<void*>(XLOG_CURRENT_SINK.get());
#undef XLOG_CURRENT_SINK
#define XLOG_CURRENT_SINK C_sink
  logprint(_TRACE, "C99 logprint() from C++: my pen is %d inches, I call him %s\n", 30, "Little Johny");
  logprint(_DBG, "C99 logprint() from C++: my pen is %d inches, I call him %s\n", 30, "Little Johny");
  logprint(_INFO, "C99 logprint() from C++: my pen is %d inches, I call him %s\n", 30, "Little Johny");
  logprint(_ERR, "C99 logprint() from C++: my pen is %d inches, I call him %s\n", 30, "Little Johny");
  logprint(_FATAL, "C99 logprint() from C++: my pen is %d inches, I call him %s\n", 30, "Little Johny");
#undef XLOG_CURRENT_SINK
  // C log from C file
  test_co_xlog_c99();

  SetsyslogSink(prev_syslog_sink);

  cout << "\n\n\n\n";
}
}

void test_co_xlog(TestInfo& test_info) {
  _test_co_xlog_with(CLoggerOpts({ 0, 0 }));
  _test_co_xlog_with(CLoggerOpts({ fLogPrintFunc, 0 }));
  _test_co_xlog_with(CLoggerOpts({ fLogPrintTid, 0}));
  _test_co_xlog_with(CLoggerOpts({ fLogPrintFunc | fLogPrintTid, 0 }));

  cout << "************ Testing xlog with min_severity=_TRACE+1337\n";
  _test_co_xlog_with(CLoggerOpts({ fLogPrintFunc | fLogPrintTid, _TRACE + 1337 }));

  cout << "************ Testing xlog with min_severity=_WARN\n";
  _test_co_xlog_with(CLoggerOpts({ fLogPrintFunc | fLogPrintTid, _WARN }));
}

void test_co_xlog_configs(TestInfo& test_info) {
  co::xlog::configs::SeverityMap comp;
  StringMap dict;
  LogConfigFromDictNoexcept log_conf;

  dict = { { "log-print-tid", "" }, {"log-sevs", "*:error;acceptor:debug"} };
  log_conf = LogConfigFromDictNoexcept(LogConfig(), dict, ConsumeAction::kDontConsume);
  DCHECK(!log_conf.GetError());
  DCHECK(log_conf.do_print_tid == true);
  comp = { { "*", _ERR }, { "acceptor", _DBG } };
  DCHECK(log_conf.module_severities.sev_map == comp);

  // trailing ';'
  dict = { { "log-print-tid", "" }, {"log-sevs", "*:error;acceptor:debug;"} };
  log_conf = LogConfigFromDictNoexcept(LogConfig(), dict, ConsumeAction::kDontConsume);
  DCHECK(!log_conf.GetError());
  DCHECK(log_conf.do_print_tid == true);
  comp = { { "*", _ERR }, { "acceptor", _DBG } };
  DCHECK(log_conf.module_severities.sev_map == comp);

  // trailing ';;;;'
  dict = { { "log-print-tid", "" }, {"log-sevs", "*:error;acceptor:debug;;;;"} };
  log_conf = LogConfigFromDictNoexcept(LogConfig(), dict, ConsumeAction::kDontConsume);
  DCHECK(!log_conf.GetError());
  DCHECK(log_conf.do_print_tid == true);
  comp = { { "*", _ERR }, { "acceptor", _DBG } };
  DCHECK(log_conf.module_severities.sev_map == comp);

  // empty log-sevs
  dict = { { "log-print-tid", "" }, {"log-sevs", ""} };
  log_conf = LogConfigFromDictNoexcept(LogConfig(), dict, ConsumeAction::kDontConsume);
  DCHECK(log_conf.do_print_tid == true);
  comp = { { "*", _ERR }, { "acceptor", _DBG } };
  DCHECK(log_conf.module_severities.sev_map.empty());

  // bad log-sevs
  dict = { { "log-print-tid", "" }, {"log-sevs", "acceptor:BAD;"} };
  log_conf = LogConfigFromDictNoexcept(LogConfig(), dict, ConsumeAction::kDontConsume);
  DCHECK(log_conf.GetError() == ConfigError(ConfigErrc::dict_error, ConfigErrorInfo()));
}

void test_co_xlog_printsinks(TestInfo& test_info) {
  // Example fancy message
  cout << "Log sinks:\n";
  map<string, Shptr<CLoggerSink>> sinks;
  EnumLogSinks(sinks);
  PrintLogSinks(cout, sinks, 2);
}

void _test_co_xlog_ringmemdevice_with(ostream& stm, size_t ring_size, size_t num_writes, bool do_flush) {
  auto ring_dev = make_shared<CLoggerOstreamRingDevice>(stm, ring_size, do_flush, true/*binary*/);
  Shptr<CLogger> logger(CreateLoggerWithDevices(CLoggerOpts(), { ring_dev }));
#define XLOG_CURRENT_SINK logger->getSink()
  Log(_INFO) << "Testing CLoggerOstreamRingDevice (ring_size=" << ring_size << ", num_writes=" << num_writes << ")\n";
  for (size_t i = 0; i < num_writes; i++) {
    string progress(i + 1, '|');
    Log(_INFO) << "[ " << i << " ] " << progress << "\n";
  }
#undef XLOG_CURRENT_SINK
}

void test_co_xlog_CLoggerOstreamRingDevice(TestInfo& test_info) {
  vector<size_t> ring_size_arr = { 0, 1, 3, 7, 53, 107, 193, 297, 511, 891 };
  //vector<size_t> ring_size_arr = { 107 };
  vector<size_t> num_writes_arr = { 15 };
  for (auto rs : ring_size_arr) {
    for (auto nw : num_writes_arr) {
      ostringstream stm;
      _test_co_xlog_ringmemdevice_with(stm, rs, nw, true/*do_flush*/);

      cout << stm.str() << "\n";
      cout << "\n\n\n";
    }
  }
}

#endif

void test_co_parse_cmdline_to_argv(TestInfo& ) {
  StringVector argv;
  StringVector expected;
  bool ok;

  syslog(_INFO) << "=== Simple test\n";
  ok = ParseCmdlineToArgv("   \"C:\\program files\\xxx.exe\" -xxx -xxx3 \"xxxing pen\" 111", argv);
  DCHECK(ok);
  cout << "What we've got:\n";
  for (auto arg : argv) {
    cout << "\t" << arg << "\n";
  }
  cout << "What we expected:\n";
  expected = { "C:\\program files\\xxx.exe", "-xxx", "-xxx3", "xxxing pen", "111" };
  for (auto exp : expected) {
    cout << "\t" << exp << "\n";
  }
  DCHECK(argv == expected);
  argv.clear();

  syslog(_INFO) << "=== DoubleQuote-not-closed error case test\n";
  ok = ParseCmdlineToArgv("a b \"c d\" \"", argv);
  DCHECK(!ok);

  syslog(_INFO) << "=== Quote-inside-DoubleQuotes test\n";
  ok = ParseCmdlineToArgv("./some_prog \"hello '123' world\"", argv);
  DCHECK(ok);
  cout << "What we've got:\n";
  for (auto arg : argv) {
    cout << "\t" << arg << "\n";
  }
  cout << "What we expected:\n";
  expected = { "./some_prog", "hello '123' world" };
  for (auto exp : expected) {
    cout << "\t" << exp << "\n";
  }
  DCHECK(argv == expected);
  argv.clear();

  syslog(_INFO) << "=== DoubleQuote-inside-Quotes test\n";
  ok = ParseCmdlineToArgv("./some_prog 'hello \"123\" world'", argv);
  DCHECK(ok);
  cout << "What we've got:\n";
  for (auto arg : argv) {
    cout << "\t" << arg << "\n";
  }
  cout << "What we expected:\n";
  expected = { "./some_prog", "hello \"123\" world" };
  for (auto exp : expected) {
    cout << "\t" << exp << "\n";
  }
  DCHECK(argv == expected);
  argv.clear();

  syslog(_INFO) << "=== MergeQuotes1 test\n";
  ok = ParseCmdlineToArgv("./some_prog arg1 'mother'\"xxxer\"", argv);
  DCHECK(ok);
  cout << "What we've got:\n";
  for (auto arg : argv) {
    cout << "\t" << arg << "\n";
  }
  cout << "What we expected:\n";
  expected = { "./some_prog", "arg1", "motherxxxer" };
  for (auto exp : expected) {
    cout << "\t" << exp << "\n";
  }
  DCHECK(argv == expected);
  argv.clear();

  syslog(_INFO) << "=== MergeQuotes2 test\n";
  ok = ParseCmdlineToArgv("./some_prog arg1 \"mother\"'xxxer'", argv);
  DCHECK(ok);
  cout << "What we've got:\n";
  for (auto arg : argv) {
    cout << "\t" << arg << "\n";
  }
  cout << "What we expected:\n";
  expected = { "./some_prog", "arg1", "motherxxxer" };
  for (auto exp : expected) {
    cout << "\t" << exp << "\n";
  }
  DCHECK(argv == expected);
  argv.clear();

  // On linux I would do:
  // $ echo ./some_prog arg1 \\"mother\\"'xxxer'
  // But this code doesn't support slashes as special chars
  // so they will be edges
  syslog(_INFO) << "=== MergeQuotes2A test\n";
  ok = ParseCmdlineToArgv("./some_prog arg1 \\\"mother\\\"'xxxer'", argv);
  DCHECK(ok);
  cout << "What we've got:\n";
  for (auto arg : argv) {
    cout << "\t" << arg << "\n";
  }
  cout << "What we expected:\n";
  expected = { "./some_prog", "arg1", "\\mother\\xxxer" };
  for (auto exp : expected) {
    cout << "\t" << exp << "\n";
  }
  DCHECK(argv == expected);
  argv.clear();

  syslog(_INFO) << "=== MergeQuotes2B test\n";
  ok = ParseCmdlineToArgv("./some_prog arg1 \\\"mother xxxer\\\"", argv);
  DCHECK(ok);
  cout << "What we've got:\n";
  for (auto arg : argv) {
    cout << "\t" << arg << "\n";
  }
  cout << "What we expected:\n";
  expected = { "./some_prog", "arg1", "\\mother xxxer\\" };
  for (auto exp : expected) {
    cout << "\t" << exp << "\n";
  }
  DCHECK(argv == expected);
  argv.clear();

  syslog(_INFO) << "=== MergeQuotes3 test\n";
  ok = ParseCmdlineToArgv("./some_prog \"mother\"'cock'jjjer", argv);
  DCHECK(ok);
  cout << "What we've got:\n";
  for (auto arg : argv) {
    cout << "\t" << arg << "\n";
  }
  cout << "What we expected:\n";
  expected = { "./some_prog", "mothercockjjjer" };
  for (auto exp : expected) {
    cout << "\t" << exp << "\n";
  }
  DCHECK(argv == expected);
  argv.clear();

  syslog(_INFO) << "=== MergeQuotes4 test\n";
  ok = ParseCmdlineToArgv("./some_prog your\"mother\"'cock'jjjer", argv);
  DCHECK(ok);
  cout << "What we've got:\n";
  for (auto arg : argv) {
    cout << "\t" << arg << "\n";
  }
  cout << "What we expected:\n";
  expected = { "./some_prog", "yourmothercockjjjer" };
  for (auto exp : expected) {
    cout << "\t" << exp << "\n";
  }
  DCHECK(argv == expected);
  argv.clear();

  // -------------------------------------------------------------------
  syslog(_INFO) << "=== Tricky cmdline tests ...\n";

  ok = ParseCmdlineToArgv("", argv);
  DCHECK(ok);
  DCHECK(argv == StringVector({}));

  ok = ParseCmdlineToArgv("'", argv);
  DCHECK(!ok);

  ok = ParseCmdlineToArgv("''", argv);
  DCHECK(ok);
  DCHECK(argv == StringVector({""}));

  ok = ParseCmdlineToArgv(" '' ", argv);
  DCHECK(ok);
  DCHECK(argv == StringVector({""}));

  ok = ParseCmdlineToArgv("\"", argv);
  DCHECK(!ok);

  ok = ParseCmdlineToArgv("\"\"", argv);
  DCHECK(ok);
  DCHECK(argv == StringVector({ "" }));

  // "  "'  '"  "
  ok = ParseCmdlineToArgv("  \"  \"'  '\"  \"  ", argv);
  DCHECK(ok);
  DCHECK(argv == StringVector({ "      " }));

  ok = ParseCmdlineToArgv("\"'", argv);
  DCHECK(!ok);

  ok = ParseCmdlineToArgv("'\"", argv);
  DCHECK(!ok);

  ok = ParseCmdlineToArgv("\" '", argv);
  DCHECK(!ok);

  ok = ParseCmdlineToArgv(" \" '", argv);
  DCHECK(!ok);

  ok = ParseCmdlineToArgv("' \"", argv);
  DCHECK(!ok);

  ok = ParseCmdlineToArgv(" ' \"", argv);
  DCHECK(!ok);
}

void test_co_strings(TestInfo& ) {

  uint32_t u;
  DCHECK(string_to_uint("1", u) && u == 1);
  DCHECK(!string_to_uint(".", u));
  DCHECK(!string_to_uint("189f", u));
  DCHECK(string_to_uint("189f", u, 16) && u == 0x189f);
  DCHECK(!string_to_uint("189f;", u, 16));

  typedef vector<string> vecstr;
  vecstr p;
  string_split("a b", " ", p);
  DCHECK(p == vecstr({"a", "b"}));
  string_split("a b ", " ", p);
  DCHECK(p == vecstr({ "a", "b", "" }));
  string_split("", "", p);
  DCHECK(p == vecstr({""}));
}

//-------------------------------------------------------------------------------------------------

void test_co_keyed_cmdline(TestInfo& ) {
  const char* argv[] = { "C:\\program files\\xxx\\xxx.exe", "abc", "arg with spaces",
    "--your-mother=ditch", "--son-of-a-gun", "scared of women with pen"};
  int argc = sizeof(argv)/sizeof(argv[0]);

  KeyedCmdLine<char> cmdline(argc, (char**)argv);

  DCHECK(cmdline.GetNamedArgs().size() == 2);
  DCHECK(cmdline.GetUnnamedArgs().size() == 3);

  DCHECK(cmdline.HasNamedArg("your-mother"));
  DCHECK(cmdline.HasNamedArg("son-of-a-gun"));
  DCHECK(cmdline.GetNamedArgs()["your-mother"] == "ditch");
  DCHECK(cmdline.GetNamedArgs()["son-of-a-gun"] == "");

  DCHECK(cmdline.GetUnnamedArgs()[0] == "abc");
  DCHECK(cmdline.GetUnnamedArgs()[1] == "arg with spaces");
  DCHECK(cmdline.GetUnnamedArgs()[2] == "scared of women with pen");
}

namespace {
static void TestCLTOn(const string& raw_cmdline,
                      bool print_prog_name = true) {
  // parse -> textualize -> parse -> compare objects
  KeyedCmdLine<char> kcl(raw_cmdline.c_str());
  string textualized;
  kcl.Textualize(textualized, print_prog_name); // def case
  if (textualized != raw_cmdline) {
    syslog(_FATAL) << "UNMATCHED!*!*!\n"
      << "source     : <" << raw_cmdline << ">\n"
      << "textualized: <" << textualized << ">\n"
      << "srclen     : " << raw_cmdline.length() << "\n"
      << "txtlen     : " << textualized.length() << "\n";

  }
  else {
    syslog(_INFO) << "[OK] match - " << raw_cmdline << "\n";
  }
  KeyedCmdLine<char> kcl2(textualized.c_str());
  DCHECK(kcl2 == kcl);
  //DCHECK(textualized == raw_cmdline); // no more text comparation
}
}

void test_co_keyed_cmdline_textualize(TestInfo& ) {

  TestCLTOn("");
  TestCLTOn("xxx.exe");
  TestCLTOn("xxx.exe abc");
  TestCLTOn("xxx.exe abc cef");
  TestCLTOn("xxx.exe --big");
  TestCLTOn("xxx.exe --big-dildo");
  TestCLTOn("xxx.exe --big-dildo=1");
  TestCLTOn("xxx.exe abc --big-dildo=1");
  TestCLTOn("xxx.exe 333 --big-dildo=1 --xxx");
  TestCLTOn("xxx.exe --sex --big-dildo=1 --xxx");
  TestCLTOn("xxx.exe -- -- --sex --big-dildo=1 --xxx");

  // print_program_name not tested, too lazy
}

void test_co_string_from_hex(TestInfo& ) {
  string b;
  DCHECK(string_from_hex("ABCD", b, 1, 4) == true);
  DCHECK(b.length() == 2);
  DCHECK(b[0] == '\xAB');
  DCHECK(b[1] == '\xCD');

  DCHECK(string_from_hex("", b, 1, 4) == false);

  DCHECK(string_from_hex("A", b, 1, 4) == false);

  DCHECK(string_from_hex("CB", b, 1, 4) == true);
  DCHECK(b.length() == 1);
  DCHECK(b[0] == '\xCB');

  DCHECK(string_from_hex("ABC", b, 1, 4) == false);

  DCHECK(string_from_hex("10AA", b, 1, 4) == true);

  DCHECK(string_from_hex("000", b, 1, 4) == false);

  DCHECK(string_from_hex("AB00X", b, 1, 4) == false);

  DCHECK(string_from_hex("AB00XX", b, 1, 4) == false);
}

void test_co_string_radix16(TestInfo&) {

  uint64_t v;

  DCHECK( string_to_uint64("deaddeedbadbeef0", v, 16) );
  DCHECK( v == 0xdeaddeedbadbeef0 );

  DCHECK( string_to_uint64("dada", v, 16) );
  DCHECK( v == 0xdada );

  DCHECK(!string_to_uint64("dada", v, 10)); // lol, try 10 instead of 16

  string s;

  v = 0x133833522209E;
  s = string_from_uint64(v, 16);
  string z = string_to_upper(s);
  DCHECK( z == "133833522209E" );
  s = string_from_uint64(v, 10);
  DCHECK( s == "338113601872030" );

  v = 3;
  s = string_from_uint64(v, 16);
  DCHECK(s == "3");
}

void test_co_cmdline_parsed_command_line(TestInfo& ) {
  ParsedCommandLine e("'abc 111.exe' 123 333 \"444 100\"");
  DCHECK(e.GetParserRet() == 0);
  DCHECK(e.GetArgc() == 4);
  DCHECK(string(e.GetArgv()[0]) == "abc 111.exe");
  DCHECK(string(e.GetArgv()[1]) == "123");
  DCHECK(string(e.GetArgv()[2]) == "333");
  DCHECK(string(e.GetArgv()[3]) == "444 100");
}

void test_co_cmdline_parsed_command_line_empty(TestInfo& ) {
  ParsedCommandLine e("");
  DCHECK(e.GetParserRet() == 0);
  DCHECK(e.GetArgc() == 0);
}

namespace  {
template <typename T>
void TestCoRandGenWith(T from, T to) {
  syslog(_INFO) << "Testing co::RandGen<> with from=" << from << ", to=" << to << "\n";

  RandGen rg(GenerateSeed());
  for (int i=0; i<100; i++) {
    size_t n = rg.RandInt<T>(from ,to);
    syslog(_INFO) << "generated number " << n << "\n";
    DCHECK(n >= from);
    DCHECK(n <= to);
  }
}
}
void test_co_rand_gen(TestInfo& ) {

  syslog(_INFO) << "co::RandGen<int> test........\n";
  TestCoRandGenWith<int>(10, 20);

  syslog(_INFO) << "co::RandGen<size_t> test........\n";
  TestCoRandGenWith<size_t>(13829, 213872823);

  syslog(_INFO) << "co::RandGen<uint64_t> test........\n";
  TestCoRandGenWith<uint64_t>(0xbeadbeaf0000000, 0xdeadbecccccccccc);

  syslog(_INFO) << "co::RandGen<uint8_t> test........\n";
  TestCoRandGenWith<uint8_t>('a', 'z');


}














