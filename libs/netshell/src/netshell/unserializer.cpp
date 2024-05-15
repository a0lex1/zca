#include "netshell/unserializer.h"
#include "netshell/detail/string_reader.h"

namespace netshell {

NsCmdResultUnserializer::NsCmdResultUnserializer(
  const NsStatusDescriptorTable& status_descriptors,
  NsCmdResult& ns_result)
  :
  untexer_(status_descriptors, ns_result)
{
}

void NsCmdResultUnserializer::Unserialize(co::BinReader& reader, NetshellError& err) {
  const uint8_t* packet;
  uint32_t packet_len;
  if (!reader.ReadByteArray(packet, &packet_len)) {
    err = NetshellErrc::unexpected_end;
    return;
  }
  detail::StringReaderFromBuffer sr(packet, packet_len); // StringReader aggregates tokenizer_
  untexer_.Untextualize(sr, err);
  if (err) {
    return;
  }
}

}

