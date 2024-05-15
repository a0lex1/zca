#include "cc/test_kit/client_proto_tester_runner.h"
#include "cc/cc_proto/all.h"

#include "proto/sync/proto_coro.h"

#include "co/async/tcp_service.h"
#include "co/async/tcp.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace cc;
using namespace boost::posix_time;
using namespace co;
using namespace co::async;
using namespace co::net;

using namespace cc::test_kit;

namespace {

class WritCmdresErrTester : public ClientProtoTester {
public:
  virtual ~WritCmdresErrTester() = default;

  using ClientProtoTester::ClientProtoTester;

  void CheckResults() {
    CcErrc e = GetCcClient().GetLastErrorStack().at(0).GetErrc();
    DCHECK(e == CcErrc::transport_error_hwcr);
  }

private:
  void DoInteraction() override {
    MustSuccess( ProtoSock().AcceptClient(TcpEndpoint("127.0.0.1", 0)));

    // use this function, which is dedicated for tests, to provoke our error
    GetCcClient().GetBotOptions()._SetInjectHandleWriteCommandResultError(true);

    Uptr<ProtoMessage> pmsg;
    MustSuccess( ProtoSock().ReadProtoMessage(pmsg) );
    DCHECK(pmsg != nullptr);
    DCHECK(pmsg->GetCode() == cc_proto::codes::kHandshake);
    auto& hmsg = pmsg->GetAs<cc_proto::HandshakeMessage>();
    syslog(_INFO) << "hrmsg.GetVersion() => " << hmsg.GetBotVersion() << "\n";

    MustSuccess(ProtoSock().WriteProtoMessage(
      cc_proto::HandshakeReplyMessage(true, 10)));

    MustSuccess(ProtoSock().WriteProtoMessage(
      cc_proto::CommandMessage(0, 0, make_shared<string>(""))));

    MustSuccess(ProtoSock().ReadProtoMessage(pmsg));
    DCHECK(pmsg != nullptr); // must fail 
    DCHECK(pmsg->GetCode() == cc_proto::codes::kCommandResult);
    auto& crmsg = pmsg->GetAs<cc_proto::CommandResultMessage>();
    DCHECK(crmsg.GetOpaqueData() == "(ok, cmd dispatched)");

    //ProtoSock().Close();
  }

  void DispatchCommand(Uptr<std::string> cmd_opaque_data,
    std::string& cmd_result_opaque_data,
    EmptyHandler handler) override
  {
    cmd_result_opaque_data = "(ok, cmd dispatched)";
    handler();
  }
};

}

void test_cc_proto_scenarios_cli_writ_cmdres_err(TestInfo&) {

  auto protfac(make_shared<cc_proto::MessageFactory>());

  WritCmdresErrTester tester;
  ClientProtoTesterRunner runner(tester, protfac);

  runner.Run();

  tester.CheckResults();
}


// ------------------------------------------------------------------------------------------------------------

namespace {

class ReadHshakeResultErrorTester : public ClientProtoTester {
public:
  virtual ~ReadHshakeResultErrorTester() = default;

  using ClientProtoTester::ClientProtoTester;

  void CheckResults() {
    DCHECK(GetCcClient().GetLastErrorStack().back().GetErrc() == CcErrc::proto_read_error_hshake_result);
  }

private:
  void DoInteraction() override {
    MustSuccess(ProtoSock().AcceptClient(TcpEndpoint("127.0.0.1", 0)));

    Uptr<ProtoMessage> pmsg;
    MustSuccess(ProtoSock().ReadProtoMessage(pmsg));
    DCHECK(pmsg != nullptr);
    DCHECK(pmsg->GetCode() == cc_proto::codes::kHandshake);
    auto& hmsg = pmsg->GetAs<cc_proto::HandshakeMessage>();
    syslog(_INFO) << "hrmsg.GetVersion() => " << hmsg.GetBotVersion() << "\n";

    // close after handshake; this will provoke CcClientSession::HandleReadHandshakeResult to fail
    ProtoSock().Close();
  }
};

}

void test_cc_proto_scenarios_cli_read_hshakeresult_err(TestInfo&) {

  auto protfac(make_shared<cc_proto::MessageFactory>());

  ReadHshakeResultErrorTester tester;
  ClientProtoTesterRunner runner(tester, protfac);

  runner.Run();

  tester.CheckResults();
}


// ------------------------------------------------------------------------------------------------------------

namespace {

class HshakeUnexpectErrorTester : public ClientProtoTester {
public:
  virtual ~HshakeUnexpectErrorTester() = default;

  using ClientProtoTester::ClientProtoTester;

  void CheckResults() {
    DCHECK(GetCcClient().GetLastErrorStack().size() == 1);
    CcErrc e = GetCcClient().GetLastErrorStack().at(0).GetErrc();
    DCHECK(e == CcErrc::hshake_reply_expected);
  }

private:
  void DoInteraction() override {
    MustSuccess( ProtoSock().AcceptClient(TcpEndpoint("127.0.0.1", 0)));

    Uptr<ProtoMessage> pmsg;
    MustSuccess( ProtoSock().ReadProtoMessage(pmsg) );
    DCHECK(pmsg != nullptr);
    DCHECK(pmsg->GetCode() == cc_proto::codes::kHandshake);
    auto& hmsg = pmsg->GetAs<cc_proto::HandshakeMessage>();
    syslog(_INFO) << "hrmsg.GetVersion() => " << hmsg.GetBotVersion() << "\n";

    // Write something else than hshake reply message
    MustSuccess(ProtoSock().WriteProtoMessage(
      cc_proto::CommandMessage(0, 0, make_shared<string>(""))));
  }
};

}

void test_cc_proto_scenarios_cli_hshake_unexpect(TestInfo&) {

  auto protfac(make_shared<cc_proto::MessageFactory>());

  HshakeUnexpectErrorTester tester;
  ClientProtoTesterRunner runner(tester, protfac);

  runner.Run();

  tester.CheckResults();
}

// ------------------------------------------------------------------------------------------------------------




