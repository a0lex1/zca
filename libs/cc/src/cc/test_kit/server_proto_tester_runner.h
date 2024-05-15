#pragma once

#include "cc/test_kit/server_proto_tester.h"

namespace cc {
namespace test_kit {

class ServerProtoTesterRunner {
public:
  ServerProtoTesterRunner(ServerProtoTester& tester,
                          Shptr<ProtoMessageFactory> protfac);

  void Run();

private:
  ServerProtoTester& tester_;
  Shptr<ProtoMessageFactory> protfac_;
};



}}

