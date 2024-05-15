#pragma once

#include "proto/proto_message_factory.h"

namespace cc {
namespace cc_proto {

class MessageFactory: public ProtoMessageFactory {
public:
  virtual ~MessageFactory() = default;

  virtual Uptr<ProtoMessage> CreateMessageForCode(ProtoMessageCode code) override;
  virtual void CreateTestMessages(std::vector<Uptr<ProtoMessage>>& all_msgs) override;
};

}}


