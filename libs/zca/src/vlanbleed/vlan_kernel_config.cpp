#include "./vlan_kernel_config.h"

using namespace std;
using namespace co;

bool VlanKernelConfig::Serialize(BinWriter& writer) const {
  static_assert(sizeof(accept_max) == 1);
  if (!writer.WriteUint8(accept_max)) {
    return false;
  }
  if (!writer.WriteUint16(queue_size)) {
    return false;
  }
  if (!writer.WriteUint32(buffer_size)) {
    return false;
  }
  return true;
}

bool VlanKernelConfig::Unserialize(BinReader& reader) {
  static_assert(sizeof(accept_max) == 1);
  if (!reader.ReadUint8(accept_max)) {
    return false;
  }
  if (!reader.ReadUint16(queue_size)) {
    return false;
  }
  if (!reader.ReadUint32(buffer_size)) {
    return false;
  }
  return true;
}


