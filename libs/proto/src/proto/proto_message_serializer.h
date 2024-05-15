#pragma once

#include "proto/proto_message.h"
#include "co/xlog/define_logger_sink.h"
#include <string>

DECLARE_XLOGGER_SINK("protmsgser", gProtoMessageSeializerLogSink);

namespace co {
class BinWriter;
}

class ProtoMessageSerializer {
public:
  ProtoMessageSerializer() {}

  bool SerializeMessage(const ProtoMessage& msg, co::BinWriter& writer);
  void SerializeMessage(const ProtoMessage& msg, std::string& append_to);
};



