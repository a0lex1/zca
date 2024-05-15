#pragma once

#include "co/common.h"
#include <atomic>

namespace co {

// UPD: this class has a strange name, why this guard is `weak` ?

// Wait-free guard
// 1 thread is holding, other threads fails to access (TryEnter() return false)
// Probably this functionality already exists in std or boost.

class WeakGuard {
public:
  WeakGuard() : entered_(false) {}

  bool TryEnter() {
    bool expected = false;
    bool desired = true;
    if (entered_.compare_exchange_strong(expected, desired)) {
      DCHECK(expected == false);
      return true;
    }
    else {
      /* busy, another thread grabbed our lock */
      return false;
    }
  }

  void Leave() {
    bool expected = true;
    bool desired = 0;
    bool ok = entered_.compare_exchange_strong(expected, desired);
    DCHECK(ok);
    DCHECK(expected == true);
  }

private:
  std::atomic<bool> entered_;
};

}

