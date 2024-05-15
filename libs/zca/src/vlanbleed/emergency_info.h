#pragma once

#include "co/base/bin_writer.h"

#include "co/common.h"

#include <cstdint>
#include <sstream>

// We are like co::Error, but without it. We need our own things here.

enum class EmergencyReason {
  protocol_violated,
  write_frame_failed,
  remote_emergency,

  invalid_reason
};

static std::string EmergencyReasonTitle(EmergencyReason r) {
  switch (r) {
  case EmergencyReason::protocol_violated: return "protocol violated";
  case EmergencyReason::write_frame_failed: return "write frame failed";
  case EmergencyReason::remote_emergency: return "remote emergency";
  case EmergencyReason::invalid_reason: return "invalid reason";
  default: NOTREACHED();
  }
  return "";
}

// ---------------------------------------------------------------------------

using ViolationTag = uint16_t;

static constexpr ViolationTag InvalidViolationTag = static_cast<ViolationTag>(-1);

static constexpr ViolationTag ViolationTag_UnexpectedConnResultFrame
  = static_cast<ViolationTag>(4501);

static constexpr ViolationTag ViolationTag_HandleLimitReached
  = static_cast<ViolationTag>(4502);

static constexpr ViolationTag ViolationTag_KConfigExpectedConnect
  = static_cast<ViolationTag>(4503);

static constexpr ViolationTag ViolationTag_KConfigNotExpectedConnect
  = static_cast<ViolationTag>(4504);

static constexpr ViolationTag ViolationTag_KConfigExpectedConnectResult
  = static_cast<ViolationTag>(4505);

static constexpr ViolationTag ViolationTag_KConfigNotExpectedConnectResult
  = static_cast<ViolationTag>(4506);

static constexpr ViolationTag ViolationTag_BadHandle
  = static_cast<ViolationTag>(4507);

struct EmergencyInfo {

  EmergencyInfo(EmergencyReason _r = EmergencyReason::invalid_reason,
                ViolationTag _vt = InvalidViolationTag,
                int wferr = 0)
    : reason(_r), violation_tag(_vt), writeframe_err(wferr)
  {
  }

  // Fields

  EmergencyReason reason{ EmergencyReason::invalid_reason };
  ViolationTag violation_tag{ InvalidViolationTag };
  int writeframe_err;

  // Routines

  std::string Textualize() const {
    std::stringstream ss;
    ss << "reason=" <<EmergencyReasonTitle(reason) << ";tag=" << violation_tag << ";wferrno=" << writeframe_err;
    return ss.str();
  }

  bool operator==(const EmergencyInfo& r) const {
    return this->reason == r.reason && this->violation_tag == r.violation_tag &&
      writeframe_err == r.writeframe_err;
  }
  bool operator!=(const EmergencyInfo& r) const {
    return !operator==(r);
  }
};

