#pragma once

#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>

#include <limits>

namespace co {

uint64_t GenerateSeed();

class RandGen {
public:
  RandGen(uint64_t seed);

  template <typename T=int>
  T RandInt(
    T from = std::numeric_limits<T>::min(),
    T to = std::numeric_limits<T>::max()
  )
  {
    boost::random::uniform_int_distribution<T> distrib_(from, to);
    return distrib_(rng_);
  }

private:
  boost::random::mt19937 rng_;
};


}


