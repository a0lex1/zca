#include "co/base/tests.h"
#include "co/xlog/configs.h"
#include "co/xlog/xlog.h"

#include <boost/filesystem.hpp>

#ifdef _NDEBUG
void sample_co_xlog_file(co::TestInfo&) {}
void sample_co_xlog(co::TestInfo& test_info) {/*u yyy*/ }
void sample_co_xlog_ring_file(co::TestInfo&) {}
#else

using namespace std;
using namespace co;
using namespace co::xlog;
using namespace co::xlog::configs;
using namespace boost::filesystem;

// DO I REALLY NEED THIS????????????????
#if 0
// Test WITH user input (new):
void sample_co_xlog(co::TestInfo& test_info) {
  LogConfig log_conf;

  // --num-messages=10
  size_t num_messages;

  vector<string> required_fields;
  try {
    // --num-messages must be present, throws otherwise
    OverrideFromDict<string, string, size_t>(test_info.opts_dict, "num-messages", num_messages, ConsumeAction::kConsume);
    // LogConfig must be parsed, throws otherwise
    log_conf = LogConfigFromDict(log_conf, test_info.opts_dict, ConsumeAction::kConsume, required_fields);
  }
  catch (std::system_error& e) {
    cout << "system_error exception: " << e.what() << "\n";
    return;
  }
  catch (ConfigException& e) {
    cout << "Config exception: " << e.what() << "\n";
    return;
  }

  if (!test_info.opts_dict.empty()) {
    cout << "Unknown options left:\n";
    for (auto it : test_info.opts_dict) {
      cout << "\t" << it.first << " => " << it.second << "\n";
    }
    return;
  }

  cout << "### Config object:\n";
  PrintLogConfig(cout, log_conf);
  cout << "\n";

  cout << "InitLogWithConfig not implemented yet, doing abort()\n";
  abort();
  //InitLogWithConfig(log_conf);

  for (size_t i = 0; i < num_messages; i++) {
    // also need print from modules
    syslog(_TRACE) << "Trace message " << i << " times\n";
    syslog(_ERR) << "Err message " << i << " times\n";
    syslog(_TRACE) << "\n";
  }
}
#else
void sample_co_xlog(co::TestInfo& test_info) {}
#endif

// ----------------------------------------------------------------------------------------------

// Tests WITHOUT user input (old):

void sample_co_xlog_file(co::TestInfo&) {

  string file_path = "sample_co_xlog_file.txt";
  std::ios_base::openmode mode = std::ios_base::out;
  bool append = false; //////////////////////////////////////////////////////////////////////////

  if (append) {
    mode |= std::ios_base::ate;
  }
  else {
    mode |= std::ios_base::trunc;
  }

  path abspath(absolute(file_path));
  syslog(_INFO) << "File path will be " << abspath << "\n";

  auto file_stream = make_shared<std::ofstream>(file_path, mode);

  if (!file_stream->good()) {
    syslog(_FATAL) << "File is not good, yyy\n";
    DCHECK(0);
  }

  Shptr<CLogger> logger = CreateLoggerWithDevices(CLoggerOpts(),
    { make_shared<CLoggerOstreamDevice>(file_stream, 256, false/*do_flush*/) });

#define XLOG_CURRENT_SINK logger->getSink()

  int i = 0;
  Log(_INFO) << "your  " << i++ << "\n";
  Log(_INFO) << "your  " << i++ << "\n";
  Log(_INFO) << "your  " << i++ << "\n";
  Log(_INFO) << "your  " << i++ << "\n";
  Log(_INFO) << "your  " << i++ << "\n";
  Log(_INFO) << "your  " << i++ << "\n";
  Log(_INFO) << "your  " << i++ << "\n";
}

void sample_co_xlog_ring_file(co::TestInfo&) {

  string file_path = "sample_co_xlog_ring_file.txt";
  std::ios_base::openmode mode = std::ios_base::out;
  bool append = false; //////////////////////////////////////////////////////////////////////////

  if (append) {
    mode |= std::ios_base::ate;
  }
  else {
    mode |= std::ios_base::trunc;
  }

  path abspath(absolute(file_path));
  syslog(_INFO) << "File path will be " << abspath << "\n";

  auto file_stream = make_shared<std::ofstream>(file_path, mode);

  if (!file_stream->good()) {
    syslog(_FATAL) << "File is not good, yyy\n";
    DCHECK(0);
  }

  Shptr<CLogger> logger = CreateLoggerWithDevices(CLoggerOpts(),
    { make_shared<CLoggerOstreamRingDevice>(file_stream, 10*1024, true/*do_flush*/, false/*binary*/) });

#define XLOG_CURRENT_SINK logger->getSink()

  for (int i = 0; i < 1000; i++) {
    Log(_INFO) << "your " << i << "\n";
  }

}

#endif
