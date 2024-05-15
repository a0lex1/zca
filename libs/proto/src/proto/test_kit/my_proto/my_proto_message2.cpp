#include "proto/test_kit/my_proto/my_proto_message2.h"
#include "co/base/bin_writer.h"
#include "co/base/bin_reader.h"

using namespace std;
using namespace co;

bool MyProtoMessage2::CompareBody(const ProtoMessage& r) const {
  const MyProtoMessage2& m(static_cast<const MyProtoMessage2&>(r));
  return this->xxx_ == m.xxx_ ;
}

bool MyProtoMessage2::SerializeBody(co::BinWriter& writer) const {
  if (!writer.WriteString(xxx_)) {
    return false;
  }
  return true;
}

bool MyProtoMessage2::UnserializeBody(co::BinReader& reader) {
  if (!reader.ReadString(xxx_)) {
    return false;
  }
  return true;
}


