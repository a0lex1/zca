#pragma once

#include <vector>

namespace co {
namespace async {

struct ThreadModelConfig {
  static const std::size_t kDefaultThreadCount = 24;

  ThreadModelConfig(bool _force_this_thread = false,
                    const std::vector<std::size_t>& _num_threads_for_group = {},
                    std::size_t _num_threads_default = kDefaultThreadCount);


  bool operator==(const ThreadModelConfig& r) const {
    return force_this_thread == r.force_this_thread &&
      num_threads_for_group == r.num_threads_for_group &&
      num_threads_default == r.num_threads_default;
  }
  bool operator!=(const ThreadModelConfig& r) const {
    return !operator==(r);
  }

  bool force_this_thread;
  std::vector<std::size_t> num_threads_for_group; // must be empty if |force_this_thread|
  std::size_t num_threads_default; // must be 1 if |force_this_thread|
};

}}


