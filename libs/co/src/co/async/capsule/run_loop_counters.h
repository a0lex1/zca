#pragma once

#include <stddef.h>

namespace co {
namespace async {
namespace capsule {

struct RunLoopCounters {
  size_t normal_exits;
  size_t cur_iter;
  size_t recovered_inits;
  size_t recovered_starts;
  size_t recovered_runs;
};


}}}
