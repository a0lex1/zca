#include "proto/proto_message_serializer.h"
#include "co/base/bin_writer.h"
#include "co/base/hex_dump.h"
#include "co/xlog/xlog.h"

DEFINE_XLOGGER_SINK("protmsgser", gProtoMessageSeializerLogSink);
#define XLOG_CURRENT_SINK gProtoMessageSeializerLogSink

using namespace std;
using namespace co;

bool ProtoMessageSerializer::SerializeMessage(const ProtoMessage& msg, co::BinWriter& writer) {
  if (!writer.WriteUint32(msg.GetCode())) {
    return false;
  }
  return
    msg.SerializeBody(writer);
}

void ProtoMessageSerializer::SerializeMessage(const ProtoMessage& msg, string& append_to) {
  co::BinWriter writer(append_to);
  writer.WriteUint32(msg.GetCode());
  bool serialize_ok = msg.SerializeBody(writer);
  DCHECK(serialize_ok);
  //Log(_TRACE) << HexDump().DoHexDump(nullptr, append_to);
}

