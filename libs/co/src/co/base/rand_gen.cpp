#include "co/base/rand_gen.h"

#include <chrono>

namespace co {

uint64_t GenerateSeed() {
  uint64_t seed = std::chrono::high_resolution_clock::now()
                                   .time_since_epoch()
                                   .count();
  return seed;
}

// ---

RandGen::RandGen(uint64_t seed): rng_(seed) {
}


}

