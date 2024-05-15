#include "co/xlog/configs/init_log_with_config.h"
#include "co/xlog/xlog.h"
#include "co/xlog/global_data.h"

using namespace std;

namespace co {
namespace xlog {
namespace configs {

void InitLogWithConfigNoexcept(const LogConfig& log_conf, LogConfigError& err) {
#ifdef CO_XLOG_DISABLE
  err = LogConfigError::NoError(); // silently 
  return;
#else
  using namespace detail;

  // Cleanup after previous session
  co::xlog::global_data::g_cur_func_name_column_padding = 0;

  // Override default CLoggerOpts from config (has its own defaults and it's probably bad)
  CLoggerOpts opts;
  if (log_conf.do_print_tid) {
    opts.format_flags |= fLogPrintTid;
  }
  else {
    opts.format_flags &= ~fLogPrintTid;
  }
  if (log_conf.do_print_func) {
    opts.format_flags |= fLogPrintFunc;
  }
  else {
    opts.format_flags &= ~fLogPrintFunc;
  }
  opts.fix_long_lambda_names = log_conf.fix_long_lambda_names;
  opts.pad_func_name = log_conf.pad_func_name;

  vector<Shptr<CLoggerDevice>> devs;
  if (log_conf.enable_stdout) {
    devs.push_back(make_shared<CLoggerStdoutDevice>());
  }
  if (log_conf.file_path != "") {
    Shptr<CLoggerDevice> file_dev;
    // First, open file. Then select device - simple or ring-buffer.

    // Make mode flags.
    std::ios_base::openmode mode = std::ios_base::out;
    if (log_conf.file_append) {
      mode |= std::ios_base::app;
    }
    else {
      mode |= std::ios_base::trunc;
    }
    auto file_stream = make_shared<std::ofstream>(log_conf.file_path, mode);
    if (!file_stream->good()) {
      err = LogConfigError(LogConfigErrc::cannot_open_output_file, LogConfigErrorInfo(log_conf.file_path));
      return;
    }
    if (log_conf.file_ring_file) {
      file_dev = make_shared<CLoggerOstreamRingDevice>(file_stream,
        log_conf.file_max_size,
        log_conf.file_do_flush,
        false/*binary*/);
    }
    else {
      file_dev = make_shared<CLoggerOstreamDevice>(file_stream,
        log_conf.file_max_size,
        log_conf.file_do_flush);
    }
    devs.push_back(file_dev);
  }

  // Make copy because we are consuming from it
  SeverityMap conf_sev_map_copy(log_conf.module_severities.sev_map);

  // Remember * and syslog if present and erase them
  int allmods_min_sev = LogConfig::kDefaultModsMinSeverity;
  int syslog_min_sev = LogConfig::kDefaultsyslogMinSeverity;
  SeverityMap::iterator it;
  if ((it = conf_sev_map_copy.find("*")) != conf_sev_map_copy.end()) {
    // Set |allmods_min_sev|
    allmods_min_sev = it->second;
    syslog_min_sev = allmods_min_sev; // by default, syslog is included in *
    conf_sev_map_copy.erase(it);
    // Check there is no another * in config
    if (conf_sev_map_copy.find("*") != conf_sev_map_copy.end()) {
      err = LogConfigError(LogConfigErrc::need_only_one_asterisk, LogConfigErrorInfo());
      return;
    }
  }
  if ((it = conf_sev_map_copy.find("syslog")) != conf_sev_map_copy.end()) {
    // Set |syslog_min_sev|
    syslog_min_sev = it->second;
    conf_sev_map_copy.erase(it);
    // Check there is no another syslog
    if (conf_sev_map_copy.find("syslog") != conf_sev_map_copy.end()) {
      err = LogConfigError(LogConfigErrc::need_only_one_syslog, LogConfigErrorInfo());
      return;
    }
  }

  // Init each module
  SinkMap db_table_copy = xlogVarTable::get().getSinkPtrMap();

  for (SeverityMap::iterator conf_it = conf_sev_map_copy.begin();
    conf_it != conf_sev_map_copy.end();
    /*|conf_it| incremented by erase()*/
    )
  {
    auto db_it = db_table_copy.find(conf_it->first);
    if (db_it == db_table_copy.end()) {
      err = LogConfigError(LogConfigErrc::module_not_found_for_severity,
        LogConfigErrorInfo(conf_it->first));
      // Clear what already inited
      UninitLogWithConfig();
      return;
    }
    opts.min_severity = conf_it->second;
    *db_it->second = CreateLoggerWithDevices(opts, devs);

    // Consume from DB
    db_table_copy.erase(db_it);
    // Consume from config
    conf_it = conf_sev_map_copy.erase(conf_it);
    if (conf_it == conf_sev_map_copy.end()) {
      break;
    }
    // |conf_it| incremented by erase()
  }

  // What is left in |conf_sev_map_copy| is unknown modules which aren't
  // found in db. This is an error case.
  if (!conf_sev_map_copy.empty()) {
    err = LogConfigError(LogConfigErrc::module_not_found_for_severity,
      LogConfigErrorInfo(conf_sev_map_copy.begin()->first));
    UninitLogWithConfig();
    return;
  }

  // Init * and syslog that we remembered
  // What is left in |db_table| are modules that aren't specified explicitly,
  // apply * rule to them
  for (auto& db_it : db_table_copy) {
    opts.min_severity = allmods_min_sev;
    *db_it.second = CreateLoggerWithDevices(opts, devs);
  }

  // Init syslog
  opts.min_severity = syslog_min_sev;
  // What is |sep_char|:
  // Default |sep_char| is ' ' (space) so logs look like this:
  // [INFO ] _test_dbserver_with_params(): tmodel.Run() returned
  // For syslog, we use ';' so syslog looks like this:
  // [INFO ];_test_dbserver_with_params(): tmodel.Run() returned
  opts.sep_char = ';';
  SetsyslogSink(CreateLoggerWithDevices(opts, devs));

  err = LogConfigError::NoError();
#endif //CO_XLOG_DISABLE
}

void InitLogWithConfig(const LogConfig& log_conf) {
#ifdef CO_XLOG_DISABLE
  return;
#else
  LogConfigError error;
  InitLogWithConfigNoexcept(log_conf, error);
  if (error) {
    throw LogConfigException(error);
  }
#endif
}

void UninitLogWithConfig() {
#ifdef CO_XLOG_DISABLE
  return;
#else
  using namespace detail;
  auto& db_table = xlogVarTable::get().getSinkPtrMap();
  for (auto& db_it : db_table) {
    db_it.second = nullptr;
  }
  SetsyslogSink(nullptr);
#endif
}

}}}



