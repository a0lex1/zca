#pragma once

#include "cc/test_kit/client_proto_tester.h"
#include "co/async/tcp_service.h"

namespace cc {
namespace test_kit {

class ClientProtoTesterRunner {
public:
  ClientProtoTesterRunner(ClientProtoTester& tester,
                          Shptr<ProtoMessageFactory> protfac);

  void Run();

private:
  co::async::ThreadModel tm_;
  co::async::TcpService svc_;
  co::RefTrackerContext rtctx_;
  Uptr<CcClient> cc_cli_;
  ClientProtoTester& tester_;
  Shptr<ProtoMessageFactory> protfac_;
};



}}

