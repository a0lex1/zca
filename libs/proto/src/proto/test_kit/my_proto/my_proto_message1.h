#pragma once

#include "proto/test_kit/my_proto/my_proto_message_codes.h"
#include "proto/proto_message.h"

#include "co/common.h"

class MyProtoMessageFactory;
class MyProtoMessage1: public ProtoMessage {
public:
  virtual ~MyProtoMessage1() = default;

  MyProtoMessage1(int apple, int banana): ProtoMessage(kMyProtoMessage1) {
    apple_ = apple;
    banana_ = banana;
  }

  co::DebugTagOwner _DbgTagForMsgCode(ProtoMessageCode code) const override {
    return MakeDebugTag("MyProtoMessage2");
  }

private:
  int apple_;
  int banana_;

private:
  bool CompareBody(const ProtoMessage&) const override;
  bool SerializeBody(co::BinWriter&) const override;
  bool UnserializeBody(co::BinReader&) override;

  friend class MyProtoMessageFactory;
  MyProtoMessage1() : MyProtoMessage1(0, 0) {}
};


