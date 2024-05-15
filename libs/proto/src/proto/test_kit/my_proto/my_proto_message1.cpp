#include "proto/test_kit/my_proto/my_proto_message1.h"
#include "co/base/bin_writer.h"
#include "co/base/bin_reader.h"

using namespace std;
using namespace co;

bool MyProtoMessage1::CompareBody(const ProtoMessage& r) const {
  const MyProtoMessage1& m(static_cast<const MyProtoMessage1&>(r));
  return this->apple_ == m.apple_ && this->banana_ == m.banana_;
}

bool MyProtoMessage1::SerializeBody(co::BinWriter& writer) const {
  if (!writer.WriteInt32(apple_)) {
    return false;
  }
  if (!writer.WriteInt32(banana_)) {
    return false;
  }
  return true;
}

bool MyProtoMessage1::UnserializeBody(co::BinReader& reader) {
  if (!reader.ReadInt32(apple_)) {
    return false;
  }
  if (!reader.ReadInt32(banana_)) {
    return false;
  }
  return true;
}


