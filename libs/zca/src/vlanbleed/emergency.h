#pragma once

#include "./emergency_info.h"

#include "co/base/error.h"
#include "co/common.h"

#include <cstdint>

class VlanAdapter;

// User must answer
class VlanEmergencyContext {
public:
  const EmergencyInfo& GetInfo() const { return info_; }

  void SetEnablePeerErrorNotify(bool enable_peer_notify) {
    enable_peer_notify_ = enable_peer_notify;
  }
  bool GetEnablePeerNotify() const { return enable_peer_notify_; }

private:
  friend class VlanAdapter;
  VlanEmergencyContext(const EmergencyInfo& info = EmergencyInfo(),
                       bool enable_peer_notify = true)
    : info_(info), enable_peer_notify_(enable_peer_notify)
  {
  }

private:
  EmergencyInfo info_;
  bool enable_peer_notify_{true}; // liberalistic default
};

// -----------------------------

using VlanEmergencyFn = Func<void(VlanEmergencyContext&)>;

// -----------------------------

