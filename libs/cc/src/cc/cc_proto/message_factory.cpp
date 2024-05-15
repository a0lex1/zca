#include "cc/cc_proto/message_factory.h"

#include "cc/cc_proto/handshake_message.h"
#include "cc/cc_proto/handshake_reply_message.h"
#include "cc/cc_proto/command_message.h"
#include "cc/cc_proto/command_result_message.h"
#include "cc/cc_proto/ping_message.h"
#include "cc/cc_proto/fire_message.h"
#include "cc/cc_proto/skipme_message.h"

using namespace std;

namespace cc {
namespace cc_proto {

Uptr<ProtoMessage> MessageFactory::CreateMessageForCode(ProtoMessageCode code) {
  switch (code) {
  case codes::kHandshake:
    return Uptr<ProtoMessage>(new HandshakeMessage);
  case codes::kHandshakeReply:
    return Uptr<ProtoMessage>(new HandshakeReplyMessage(false, 0));
  case codes::kCommand:
    return Uptr<ProtoMessage>(
      new CommandMessage(0/*seq*/, 0/*sig*/, make_unique<string>()));
  case codes::kCommandResult:
    return Uptr<ProtoMessage>(
      new CommandResultMessage(0, make_unique<string>()));
  case codes::kPing:
    return Uptr<ProtoMessage>(new PingMessage);
  case codes::kFire:
    return Uptr<ProtoMessage>(new FireMessage);
  case codes::kSkipMe:
    return Uptr<ProtoMessage>(new SkipMeMessage);
    default:
      return nullptr;
  }
}


void MessageFactory::CreateTestMessages(vector<Uptr<ProtoMessage>>& all_msgs) {

  all_msgs.push_back(CreateMessageForCode(codes::kHandshake));
  all_msgs.push_back(CreateMessageForCode(codes::kHandshakeReply));
  all_msgs.push_back(CreateMessageForCode(codes::kCommand));
  all_msgs.push_back(CreateMessageForCode(codes::kCommandResult));
  all_msgs.push_back(CreateMessageForCode(codes::kPing));
  all_msgs.push_back(CreateMessageForCode(codes::kFire));
  all_msgs.push_back(CreateMessageForCode(codes::kSkipMe));

}

}}

