#include "proto/test_kit/proto_serialization_test.h"
#include "proto/proto_message_serializer.h"
#include "proto/proto_message_unserializer.h"

using namespace std;

void ProtoSerializationTest(ProtoMessageFactory& fac) {
  vector<Uptr<ProtoMessage>> msgs;
  fac.CreateTestMessages(msgs);
  for (size_t i=0; i<msgs.size(); i++) {
    string sered;
    ProtoMessageSerializer serer;
    serer.SerializeMessage(*msgs[i], sered);
    Uptr<ProtoMessage> unsered;
    ProtoMessageUnserializer unser(fac);
    unsered = unser.UnserializeMessage(sered);
    DCHECK(unsered != nullptr);

    DCHECK(msgs[i]->Compare(*unsered.get()));
  }
}


