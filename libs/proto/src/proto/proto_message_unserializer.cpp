#include "proto/proto_message_unserializer.h"
#include "co/base/bin_reader.h"
#include "co/base/hex_dump.h"
#include "co/xlog/xlog.h"

DEFINE_XLOGGER_SINK("protomsgunser", gProtoMsgUnserializerLogSink);
#define XLOG_CURRENT_SINK gProtoMsgUnserializerLogSink

using namespace std;

bool ProtoMessageUnserializer::CheckCodeAllowed(
  ProtoMessageCode code,
  const vector<ProtoMessageCode>* allowed_messages)
{
  return
    std::find(allowed_messages->begin(), allowed_messages->end(), code)
    !=
    allowed_messages->end();
}

Uptr<ProtoMessage> ProtoMessageUnserializer::UnserializeMessage(
  co::BinReader& reader,
  const vector<ProtoMessageCode>* allowed_messages)
{
  ProtoMessageCode code;
  if (!reader.ReadUint32(code)) {
    Log(_TRACE) << "ReadUint32(code) failed\n";
    return nullptr;
  }
  if (allowed_messages != nullptr && !CheckCodeAllowed(code, allowed_messages)) {
    Log(_TRACE) << "code is not allowed\n";
    return nullptr;
  }
  Uptr<ProtoMessage> msg = msg_fac_.CreateMessageForCode(code);
  if (!msg) {
    Log(_TRACE) << "CreateMessageFromCode() = nullptr";
    return nullptr;
  }
  if (!msg->UnserializeBody(reader)) {
    Log(_TRACE) << "UnserializeBody() failed\n";
    return nullptr;
  }
  return msg;
}

Uptr<ProtoMessage> ProtoMessageUnserializer::UnserializeMessage(const uint8_t* data, uint32_t data_len, const std::vector<ProtoMessageCode>* allowed_messages /*= nullptr*/) {
  co::BinReader reader(data, data_len);
  return UnserializeMessage(reader);
}


