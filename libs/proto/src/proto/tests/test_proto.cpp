#include "proto/test_kit/proto_serialization_test.h"
#include "proto/test_kit/proto_test_through_messagerw.h"
#include "proto/test_kit/my_proto/my_proto_message_factory.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace co;

void test_proto(TestInfo& test_info) {
  syslog(_INFO) << "testing MyProto...\n";
  MyProtoMessageFactory fac;

  syslog(_INFO) << "serialization test...\n";
  ProtoSerializationTest(fac);

  syslog(_INFO) << "message r/w socket test...\n";
  ProtoTestThroughMessageRw(fac);
}

