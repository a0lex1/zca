#pragma once

#include "co/xlog/configs/log_config.h"
#include "co/xlog/configs/log_config_error.h"
#include "co/xlog/configs/log_config_exception.h"
#include "co/common.h"

namespace co {
namespace xlog {
namespace configs {

// Throws LogConfigException
void InitLogWithConfigNoexcept(const LogConfig& log_conf, LogConfigError& err);
void InitLogWithConfig(const LogConfig& log_conf); // throws
void UninitLogWithConfig();

}}}





