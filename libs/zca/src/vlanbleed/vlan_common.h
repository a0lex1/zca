#pragma once

#include "./error.h"

#include "co/common.h"

using HandlerWithVlErr = Func<void(const VlanError& vlerr)>;
using HandlerWithVlErrSize = Func<void(const VlanError&, size_t num_bytes)>;

