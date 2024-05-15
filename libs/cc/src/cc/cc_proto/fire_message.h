#pragma once

#include "cc/cc_proto/codes.h"

#include "proto/proto_message.h"

namespace cc {
namespace cc_proto {

class FireMessage: public ProtoMessage {
  static const ProtoMessageCode kCode = codes::kFire;
public:
  virtual ~FireMessage() = default;

  FireMessage(): ProtoMessage(kCode) {
  }

  co::DebugTagOwner _DbgTagForMsgCode(ProtoMessageCode code) const override {
    return MakeDebugTag("cc_proto::FireMessage");
  }

private:
  bool CompareBody(const ProtoMessage& r) const override ;
  bool SerializeBody(co::BinWriter& writer) const override;
  bool UnserializeBody(co::BinReader& reader) override;
};

}}

