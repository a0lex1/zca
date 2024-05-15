#pragma once

#include "co/common.h"

namespace co {

template <typename T>
class Adaptor {
public:
  virtual ~Adaptor() = default;

  Adaptor(Uptr<T> adapted_uptr) {
    adapted_ = std::move(adapted_uptr);
    adapted_ptr_ = adapted_.get();
  }
  Adaptor(T& adapted_ref) {
    adapted_ptr_ = &adapted_ref;
  }
protected:
  T& GetAdaptedObject() { return *adapted_ptr_; }
private:
  Uptr<T> adapted_;
  T* adapted_ptr_;
};


}


