#pragma once

#include <cstdint>

namespace co {

namespace common_config {

static const uint32_t kMaxChunkBodySize = 60 * 1024;
static const uint32_t kLineReaderMaxLineLen = 32768; // 32kib

}

}
