#pragma once

#include "cc/cc_proto/codes.h"

#include "proto/proto_message.h"

#include "co/common.h"

#include <cstdint>

namespace cc {
namespace cc_proto {

class CommandResultMessage: public ProtoMessage
{
  static const ProtoMessageCode kCode = codes::kCommandResult;
public:
  virtual ~CommandResultMessage() = default;

  CommandResultMessage(uint32_t seq_num, Uptr<std::string> opaque_data) 
    :
    ProtoMessage(kCode),
    seq_num_(seq_num),
    opaque_data_(std::move(opaque_data))
  {
  }

  const std::string& GetOpaqueData() const {
    return *opaque_data_.get();
  }

  co::DebugTagOwner _DbgTagForMsgCode(ProtoMessageCode code) const override {
    return MakeDebugTag("cc_proto::CommandResultMessage");
  }

private:
  uint32_t seq_num_; // who calls us, need init:??????//
  Uptr<std::string> opaque_data_;


private:
  bool CompareBody(const ProtoMessage& r) const override;
  bool SerializeBody(co::BinWriter& writer) const override;
  bool UnserializeBody(co::BinReader& reader) override;

  friend class ProtoMessageUnserializer;
  CommandResultMessage(): ProtoMessage(kCode) {}
};

}}

