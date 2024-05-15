#include "cc/cc_proto/command_result_message.h"

#include "co/base/bin_reader.h"
#include "co/base/bin_writer.h"

using namespace std;
using namespace co;

namespace cc {
namespace cc_proto {

bool CommandResultMessage::SerializeBody(co::BinWriter& writer) const {
  if (!writer.WriteUint32(seq_num_)) {
    return false;
  }
  if (!writer.WriteString(*opaque_data_.get())) {
    return false;
  }
  return true;
}

bool CommandResultMessage::UnserializeBody(co::BinReader& reader) {
  if (!reader.ReadUint32(seq_num_)) {
    return false;
  }
  if (!reader.ReadString(*opaque_data_)) {
    return false;
  }
  return true;
}

bool CommandResultMessage::CompareBody(const ProtoMessage& r) const {
  return
    this->GetOpaqueData() ==
      static_cast<const CommandResultMessage&>(r).GetOpaqueData();
}

}}

