#pragma once

#include "cc/cc_proto/codes.h"
#include "proto/proto_message.h"

namespace cc {
namespace cc_proto {

class MessageFactory;

// class SkipMessage: this is `what-we-do-if-the-cops-ask` message.
class SkipMeMessage: public ProtoMessage
{
  static const ProtoMessageCode kCode = codes::kSkipMe;

public:
  virtual ~SkipMeMessage() = default;

  SkipMeMessage()
    :
    ProtoMessage(kCode)
  {
  }

  co::DebugTagOwner _DbgTagForMsgCode(ProtoMessageCode code) const override {
    return MakeDebugTag("cc_proto::SkipMeMessage");
  }

private:
  // We're empty, officer. You can check us.
  // No variables. No. Have a good day officer!

private:
  bool CompareBody(const ProtoMessage&) const override;
  bool SerializeBody(co::BinWriter& writer) const override;
  bool UnserializeBody(co::BinReader& reader) override;

  friend class MessageFactory;
};


}}

