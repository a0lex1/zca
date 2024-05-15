#pragma once

#include "cc/cc_proto/codes.h"

#include "proto/proto_message.h"

#include "co/common.h"

namespace cc {
namespace cc_proto {

class MessageFactory;

class CommandMessage
  :
  public ProtoMessage
{
  static const ProtoMessageCode kCode = codes::kCommand;

public:
  virtual ~CommandMessage() = default;

  CommandMessage(uint32_t seq_num, int signature, Shptr<std::string> opaque_data)
    :
    ProtoMessage(kCode),
    seq_num_(seq_num),
    signature_(signature),
    opaque_data_(move(opaque_data))
  {
  }

  uint32_t GetSequenceNumber() const {
    return seq_num_;
  }
  int GetSignature() const {
    return signature_;
  }
  const std::string& GetOpaqueData() const {
    return *opaque_data_.get();
  }

  co::DebugTagOwner _DbgTagForMsgCode(ProtoMessageCode code) const override {
    return MakeDebugTag("cc_proto::CommandMessage");
  }


private:
  uint32_t seq_num_; // do I need {0} here?? who creates this class?
  int signature_; // who does create this clas???
  Shptr<std::string> opaque_data_;

private:
  // [ProtoMessage impl]
  bool CompareBody(const ProtoMessage& r) const override;
  bool SerializeBody(co::BinWriter& writer) const override;
  bool UnserializeBody(co::BinReader&) override;
};

}}


