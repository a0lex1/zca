#pragma once

#include "co/base/opaque_object.h"

//
// Opaque Strategy Context (can encapsulate either Stream or anything like VlanNative API's channel/handle)
//

class StrategyContext : public co::OpaqueObject {
 public:
  virtual ~StrategyContext() = default;
};

