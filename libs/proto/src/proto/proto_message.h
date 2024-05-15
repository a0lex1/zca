#pragma once

#include "proto/proto_message_code.h"
#include "co/common.h"

namespace co {
class BinReader;
class BinWriter;
}
class ProtoMessageUnserializer;
class ProtoMessageSerializer;
class ProtoMessage {

  friend class ProtoMessageUnserializer;
  friend class ProtoMessageSerializer;

  virtual bool CompareBody(const ProtoMessage&) const = 0;
  virtual bool UnserializeBody(co::BinReader&) = 0;
  virtual bool SerializeBody(co::BinWriter&) const = 0;

public:
  virtual ~ProtoMessage() = default;

  ProtoMessage(ProtoMessageCode code) : code_(code) {}

  virtual co::DebugTagOwner _DbgTagForMsgCode(ProtoMessageCode code) const = 0;

  // syntax sugar
  template <typename T>
  const T& GetAs() const {
    return static_cast<const T&>(*this);
  }

  template <typename T>
  T& GetAs() {
    return const_cast<T&>(static_cast<const ProtoMessage*>(this)->GetAs<T>());
  }

  ProtoMessageCode GetCode() const { return code_; }

  // If you want to spoil the code, for example.
  void SetCode(ProtoMessageCode code) { code_ = code; }

  bool Compare(const ProtoMessage& r) const {
    return GetCode() == r.GetCode() && CompareBody(r);
  }
  
  bool operator==(const ProtoMessage&) = delete;

private:
  ProtoMessageCode code_;
};

