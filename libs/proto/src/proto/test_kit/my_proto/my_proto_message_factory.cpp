#include "proto/test_kit/my_proto/my_proto_message_factory.h"
#include "proto/test_kit/my_proto/my_proto_message1.h"
#include "proto/test_kit/my_proto/my_proto_message2.h"

Uptr<ProtoMessage> MyProtoMessageFactory::CreateMessageForCode(ProtoMessageCode code) {
  switch (code) {
  case kMyProtoMessage1:
    return Uptr<MyProtoMessage1>(new MyProtoMessage1());
  case kMyProtoMessage2:
    return Uptr<MyProtoMessage2>(new MyProtoMessage2());
  }
  return 0;
}

void MyProtoMessageFactory::CreateTestMessages(std::vector<Uptr<ProtoMessage>>& all_msgs) {
  all_msgs.push_back(CreateMessageForCode(kMyProtoMessage1));
  all_msgs.push_back(CreateMessageForCode(kMyProtoMessage2));
}


