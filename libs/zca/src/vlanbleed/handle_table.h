#pragma once

#include "co/common.h"

#include <cstdint>
#include <map>

// Htype must be unsigned
template <typename Htype, typename Dtype>
class HandleTable {
public:
  HandleTable(Htype max_handles) : max_handles_(max_handles) {}

  Htype GetHandleCount() const {
    return handles_.size();
  }
  bool CreateHandle(Htype& new_handle, const Dtype& new_handle_data) {
    if (!CanCreateHandle()) {
      return false;
    }
    new_handle = 0;
    for (auto& handle : handles_) {
      if (handle.first != new_handle) {
        // Numeric hole found, use it
        break;
      }
      // If no holes found, for() loop will exit and |new_handle| will be the next handle
      // We are guaranteed to have at least one empty handle.
      new_handle += 1;
    }
    handles_.insert(std::pair<Htype, Dtype>(new_handle, new_handle_data));
    return true;
  }
  bool CloseHandle(Htype handle) {
    auto it = handles_.find(handle);
    if (it == handles_.end()) {
      return false;
    }
    handles_.erase(it);
    return true;
  }
  void CloseAllHandles(Htype* num_closed = nullptr) {
    size_t size_was = handles_.size();
    handles_.clear();
    if (num_closed) {
      *num_closed = size_was;
    }
  }
  Dtype* FindHandleData(Htype handle) {
    for (auto& rec : handles_) {
      if (rec.first == handle) {
        return &rec.second;
      }
    }
    return nullptr;
  }
  bool CanCreateHandle() const {
    if (handles_.size() < max_handles_) {
      return true;
    }
    else {
      if (handles_.size() == max_handles_) {

      }
      else {
        DCHECK(!"wrong state");
      }
    }
    return false;
  }

private:
  Htype max_handles_;
  std::map<Htype, Dtype> handles_;
};




