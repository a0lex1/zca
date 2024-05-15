#pragma once

#include "vlanbleed2/async/test_kit/stm/script_op.h"

#include <vector>

// a Script (now vector of operations +)
class Script
{
  std::vector<SOp> ops_;
 public:
  virtual ~Script() = default;

  Script(const std::vector<SOp>& ops = {})
    : ops_(ops)
  {
  }

  void AddOp(const SOp& op) {
    ops_.push_back(op);
  }
  const std::vector<SOp>& GetOps() const { return ops_; }
};


