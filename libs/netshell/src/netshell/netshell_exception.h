#pragma once

#include "netshell/netshell_error.h"

#include "co/base/error_exception.h"

namespace netshell {

class NetshellException : public ::co::ErrorException<NetshellError> {
  using ErrorException::ErrorException;
};

}
