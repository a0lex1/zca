#pragma once

#include "cc/cc_proto/codes.h"
#include "proto/proto_message.h"

#include <cstdint>

namespace cc {
namespace cc_proto {

class PingMessage: public ProtoMessage {
  static const ProtoMessageCode kCode = codes::kPing;
public:
  virtual ~PingMessage() = default;

  PingMessage(): ProtoMessage(kCode) {
  }

  co::DebugTagOwner _DbgTagForMsgCode(ProtoMessageCode code) const override {
    return MakeDebugTag("cc_proto::PingMessage");
  }

private:
  bool CompareBody(const ProtoMessage& r) const override ;
  bool SerializeBody(co::BinWriter& writer) const override;
  bool UnserializeBody(co::BinReader& reader) override;
};

}}

