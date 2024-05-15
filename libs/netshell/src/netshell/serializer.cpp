#include "netshell/serializer.h"

using namespace std;

namespace netshell {

NsCmdResultSerializer::NsCmdResultSerializer(
  const NsStatusDescriptorTable& status_descriptors,
  const NsCmdResult& ns_result)
  :
  ns_texer_(status_descriptors, ns_result)
{
}

bool NsCmdResultSerializer::Serialize(co::BinWriter& writer) {
  string packet;
  ns_texer_.Textualize(packet);
  return
    writer.WriteByteArrayCastSize(packet.c_str(), packet.length());
}


}

