#pragma once

#include <stddef.h>

namespace co {
namespace async {
namespace capsule {

class LoopOptionsBase {
public:
  static constexpr size_t kUnlimitedIterations = -1;

  virtual ~LoopOptionsBase() = default;

  virtual size_t GetMaxTotalIterations() const = 0;
  virtual size_t GetMaxNormalExitIterations() const = 0;
};

class LoopOptions : public LoopOptionsBase {
public:
  virtual ~LoopOptions() = default;

  LoopOptions(size_t max_total_iterations, size_t max_normal_exit_iters)
    : max_total_iters_(max_total_iterations), max_normal_exit_iters_(max_normal_exit_iters)
  {
  }
  size_t GetMaxTotalIterations() const override {
    return max_total_iters_;
  }
  size_t GetMaxNormalExitIterations() const override {
    return max_normal_exit_iters_;
  }
private:
  size_t max_total_iters_;
  size_t max_normal_exit_iters_;
};

class DefaultLoopOptions : public LoopOptions {
public:
  static const size_t kDefaultMaxTotalIterations = LoopOptionsBase::kUnlimitedIterations;
  static const size_t kDefaultMaxNormalExitIterations = LoopOptionsBase::kUnlimitedIterations;

  virtual ~DefaultLoopOptions() = default;

  DefaultLoopOptions() : LoopOptions(kDefaultMaxTotalIterations, kDefaultMaxNormalExitIterations)
  {
  }
};

}}}

