#pragma once

#include "co/xlog/configs/module_severity_map.h"
#include "co/xlog/define_logger_sink.h"
#include "co/xlog/severity_defs.h"
#include "co/common.h"
#include <vector>
#include <map>
#include <string>

namespace co {
namespace xlog {
namespace configs {

struct LogConfig {
  LogConfig();

  void SetDefault();

#ifndef CO_XLOG_DISABLE
  static const int kDefaultModsMinSeverity = _INFO;
  static const int kDefaultsyslogMinSeverity = _INFO;

  // --log-print-tid
  bool do_print_tid;

  // --log-print-func
  bool do_print_func;

  bool fix_long_lambda_names;

  bool fix_long_anonymous_names;

  bool pad_func_name;

  // --log-sevs=*:error;syslog:trace;acceptor:dbg;client:trace;dbserver:trace
  ModuleSeverityMap module_severities;

  // --log-stdout
  bool enable_stdout;

  // --log-file=C:\\file.txt
  std::string file_path;

  // --log-ring-file=C:\\file.txt
  bool file_ring_file;

  // --log-file-flush
  bool file_do_flush;

  // --log-file-max=31902
  size_t file_max_size;

  // --log-file-append
  bool file_append;

#endif //CO_XLOG_DISABLE
};

void PrintLogConfig(std::ostream& stm, const LogConfig& log_conf);

}}

#ifndef CO_XLOG_DISABLE
bool ConvertValue(const std::string& str, co::xlog::configs::ModuleSeverityMap& val);
#endif

}





