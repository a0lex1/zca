#pragma once

#include "co/common.h"

namespace co {

template <typename T>
class InterlockedHolder {
public:
  InterlockedHolder() {
    destroyed_ = true;
  }
  InterlockedHolder(const T& initial_data)
  {
    StoreData(initial_data);
  }
  bool LoadData(T& data) const {
    auto shptr = aptr_.load();
    if (shptr != nullptr) {
      data = *shptr;
      return true;
    }
    return false;
  }
  void StoreData(const T& data) {
    auto shptr = boost::make_shared<T>(data);
    aptr_.store(shptr);
  }
  void SafelyChangeData(Func<void(T&)> changer_func) {
    T data_copy;
    if (!LoadData(data_copy)) {
      return;
    }
    changer_func(data_copy);
    StoreData(data_copy);
  }
private:
  AtomicShptr<T> aptr_;
  bool destroyed_{ false };
};

}

