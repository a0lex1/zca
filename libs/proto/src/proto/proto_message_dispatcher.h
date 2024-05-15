#pragma once

#include "proto/proto_message.h"
#include "co/common.h"

class ProtoMessageDispatcher {
public:
  virtual ~ProtoMessageDispatcher() = default;

  // ProtoMessageDispatcher impl.
  // DispatchProtoMessage() must not directly call |handler|
  virtual void DispatchProtoMessage(const ProtoMessage& msg, HandlerWithErrcode handler) = 0;
};



