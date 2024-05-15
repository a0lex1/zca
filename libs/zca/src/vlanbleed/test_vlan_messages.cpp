#include "./messages.h"
#include "proto/test_kit/proto_serialization_test.h"
#include "proto/test_kit/proto_test_through_messagerw.h"
#include "proto/test_kit/my_proto/my_proto_message_factory.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace co;
using namespace std;

void test_vlan_messages(TestInfo& ) {

  syslog(_INFO) << "testing Vlan Proto ...\n";
  VlanProtoMessageFactory fac;

  syslog(_INFO) << "Vlan Proto serialization test...\n";
  ProtoSerializationTest(fac);

  syslog(_INFO) << "Vlan Proto message r/w socket test...\n";
  ProtoTestThroughMessageRw(fac);

}
