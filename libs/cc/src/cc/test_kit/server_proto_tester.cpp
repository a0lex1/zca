#include "cc/test_kit/server_proto_tester.h"

#include "co/xlog/xlog.h"

namespace cc {
namespace test_kit {

void ServerProtoTester::MustSuccess(Errcode e)
{
  if (e) {
    syslog(_ERR) << "**************** MustSuccess(Errcode) RAPED TO DEATH ****************\n"
      "<<<Errcode => " << e.message() << ">>>\n";
  }
}

void ServerProtoTester::MustSuccess(ProtoError pe)
{
  if (pe) {
    syslog(_ERR) << "**************** MustSuccess(ProtoError) RAPED TO DEATH ****************\n"
      "<<<ProtoError::MakeErrorMessage => " << pe.MakeErrorMessage() << ">>>\n";
  }
}

}}

