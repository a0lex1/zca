#pragma once

#include "proto/proto_message.h"
#include "co/common.h"

class ProtoMessageFactory {
public:
  virtual ~ProtoMessageFactory() = default;

  virtual Uptr<ProtoMessage> CreateMessageForCode(ProtoMessageCode code) = 0;
  virtual void CreateTestMessages(std::vector<Uptr<ProtoMessage>>& all_msgs) = 0;
};


