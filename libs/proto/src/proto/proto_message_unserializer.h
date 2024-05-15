#pragma once

#include "proto/proto_message_factory.h"
#include "co/xlog/define_logger_sink.h"
#include <limits>

namespace co {
class BinReader;
}

DECLARE_XLOGGER_SINK("protomsgunser", gProtoMsgUnserializerLogSink);

class ProtoMessageUnserializer {
public:
  ProtoMessageUnserializer(ProtoMessageFactory& msg_fac)
    : msg_fac_(msg_fac)
  {
  }
  Uptr<ProtoMessage> UnserializeMessage(co::BinReader& reader,
                                        const std::vector<ProtoMessageCode>* allowed_messages = nullptr);

  Uptr<ProtoMessage> UnserializeMessage(const uint8_t* data,
                                        uint32_t data_len,
                                        const std::vector<ProtoMessageCode>* allowed_messages = nullptr);

  Uptr<ProtoMessage> UnserializeMessage(const std::string& serialized,
                                        const std::vector<ProtoMessageCode>* allowed_messages = nullptr)
  {
    DCHECK(serialized.length() <= std::numeric_limits<uint32_t>::max());
    return UnserializeMessage(reinterpret_cast<const uint8_t*>(serialized.c_str()),
                              static_cast<uint32_t>(serialized.length()),
                              allowed_messages);
  }
private:
  static bool CheckCodeAllowed(ProtoMessageCode, const std::vector<ProtoMessageCode>*);

private:
  ProtoMessageFactory& msg_fac_;
};


