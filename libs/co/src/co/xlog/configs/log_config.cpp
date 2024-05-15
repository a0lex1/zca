#include "co/xlog/configs/log_config.h"

using namespace std;

namespace co {
namespace xlog {
namespace configs {

void PrintLogConfig(ostream& stm, const LogConfig& log_conf) {

}

void LogConfig::SetDefault()
{
#ifndef CO_XLOG_DISABLE
  do_print_tid = true; // was false)
  do_print_func = false; // was true, i'm serious
  fix_long_lambda_names = true;
  fix_long_anonymous_names = true;
  pad_func_name = true;

  module_severities.sev_map.clear();

  enable_stdout = true;
  file_path = ""; // disable
  file_ring_file = false;
  file_do_flush = true;
  file_max_size = 512 * 1024; // 512 KiB
  file_append = false;
#endif
}

LogConfig::LogConfig()
{
  SetDefault();
}

}}}



