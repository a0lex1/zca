#include "cc/cc_proto/command_message.h"
#include "co/base/bin_reader.h"
#include "co/base/bin_writer.h"
#include "co/common.h"

using namespace std;
using namespace co;

namespace cc {
namespace cc_proto {

bool CommandMessage::SerializeBody(co::BinWriter& writer) const {
  if (!writer.WriteUint32(seq_num_)) {
    return false;
  }
  if (!writer.WriteInt32(signature_)) {
    return false;
  }
  if (opaque_data_) {
    auto opaque_len(opaque_data_->length());
    DCHECK(opaque_len <= numeric_limits<uint32_t>::max());
    return writer.WriteString(*opaque_data_.get());
  }
  return true;
}

bool CommandMessage::UnserializeBody(co::BinReader& reader) {
  if (!reader.ReadUint32(seq_num_)) {
    return false;
  }
  if (!reader.ReadInt32(signature_)) {

  }
  if (!reader.ReadString(*opaque_data_.get())) {
    return false;
  }
  return true;
}

bool CommandMessage::CompareBody(const ProtoMessage& r) const {
  return
    this->GetOpaqueData() == static_cast<const CommandMessage&>(r).GetOpaqueData();
}

}}
