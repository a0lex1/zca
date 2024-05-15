#pragma once

#include "proto/test_kit/my_proto/my_proto_message_codes.h"
#include "proto/proto_message.h"

class MyProtoMessageFactory;
class MyProtoMessage2: public ProtoMessage {
public:
  virtual ~MyProtoMessage2() = default;

  MyProtoMessage2(const std::string& xxx): ProtoMessage(kMyProtoMessage2) {
    xxx_ = xxx;
  }

  co::DebugTagOwner _DbgTagForMsgCode(ProtoMessageCode code) const {
    return MakeDebugTag("MyProtoMessage2");
  }

private:
  std::string xxx_;

private:
  virtual bool CompareBody(const ProtoMessage&) const;
  virtual bool SerializeBody(co::BinWriter&) const;
  virtual bool UnserializeBody(co::BinReader&);

  friend class MyProtoMessageFactory;
  MyProtoMessage2() : MyProtoMessage2("penxxx") {}
};


