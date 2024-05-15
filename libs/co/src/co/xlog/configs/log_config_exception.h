#pragma once

#include "co/xlog/configs/log_config_error.h"

#include "co/base/error_exception.h"

namespace co {
namespace xlog {
namespace configs {

class LogConfigException : public ErrorException<LogConfigError> {
  using ErrorException::ErrorException;
};

}}}

