#pragma once

#include "proto/proto_message_code.h"

namespace cc {
namespace cc_proto {

namespace codes {
static const ProtoMessageCode kHandshake      = 1; // client to server
static const ProtoMessageCode kHandshakeReply = 2; // server to client
static const ProtoMessageCode kCommand        = 3; // server to client
static const ProtoMessageCode kCommandResult  = 4; // client to server
static const ProtoMessageCode kPing           = 5; // client to server
static const ProtoMessageCode kFire      = 6; // client to server

static const ProtoMessageCode kSkipMe         = 7; //
}

}}

