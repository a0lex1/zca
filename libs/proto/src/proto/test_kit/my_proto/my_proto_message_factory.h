#pragma once

#include "proto/proto_message_factory.h"

class MyProtoMessageFactory : public ProtoMessageFactory {
public:
  virtual ~MyProtoMessageFactory() = default;

  virtual Uptr<ProtoMessage> CreateMessageForCode(ProtoMessageCode code) override;
  virtual void CreateTestMessages(std::vector<Uptr<ProtoMessage>>& all_msgs) override;
};

