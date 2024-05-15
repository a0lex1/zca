#include "cc/test_kit/server_proto_tester_runner.h"
#include "cc/cc_proto/all.h"
#include "cc/cc_server.h"

#include "proto/test_kit/my_proto/my_proto_message_codes.h"
#include "proto/test_kit/my_proto/my_proto_message_factory.h"
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

/*

+ Handshake, handshake
+ Handshake, CmdResultMessage
+ CreateTestMessages() except handshake

*/

namespace {

class SimpleHandshakeTester : public ServerProtoTester {
public:
  virtual ~SimpleHandshakeTester() = default;

  using ServerProtoTester::ServerProtoTester;

  void CheckSomeResults() {
    // example
  }

private:
  void DoInteraction() override {
    MustSuccess( ProtoSock().ConnectServer() );

    MustSuccess( ProtoSock().WriteProtoMessage(
      cc_proto::HandshakeMessage(BotId::FromUint(13), 1, "opaqueData1")));

    Uptr<ProtoMessage> pmsg;
    MustSuccess( ProtoSock().ReadProtoMessage(pmsg) );
    DCHECK(pmsg != 0);
    DCHECK(pmsg->GetCode() == cc_proto::codes::kHandshakeReply);
    auto& hrmsg = pmsg->GetAs<cc_proto::HandshakeReplyMessage>();
    syslog(_INFO) << "hrmsg.GetSuccess() => " << hrmsg.GetSuccess() << "\n";

    ProtoSock().Close();
  }
};


class TwoSequentialHandshakesTester : public ServerProtoTester {
public:
  virtual ~TwoSequentialHandshakesTester() = default;

  using ServerProtoTester::ServerProtoTester;

  void CheckResults() {
    DCHECK(hshaked_botids_.size() == 1);
    DCHECK(hshaked_botids_[0].ToStringRepr() == "00000000000000000000000000000007");
    DCHECK(removed_errvecs_.size() == 1);
    // must be 1 `removal` operation with
    // a vector of any count of operations, but the first one must be
    // second_handshake. Others can appear in future when the new async code
    // is added (new callbacks may push extra errors, but again, the first
    // one is needed to be second_handshake).
    syslog(_INFO) << "TwoSeqHshakeTester: " << removed_errvecs_[0][0].MakeErrorMessage() << "\n";
    DCHECK(removed_errvecs_[0].size() >= 1);
    DCHECK(removed_errvecs_[0][0].GetErrc() == CcErrc::second_handshake);
  }

private:
  vector<BotId> hshaked_botids_;
  vector<vector<CcError>> removed_errvecs_;
 
private:
  void DoInteraction() override {
    MustSuccess(ProtoSock().ConnectServer());

    MustSuccess(ProtoSock().WriteProtoMessage(
      cc_proto::HandshakeMessage(BotId::FromUint(7), 6200, "opaqueHshakeData")));

    Uptr<ProtoMessage> pmsg;
    MustSuccess(ProtoSock().ReadProtoMessage(pmsg));
    DCHECK(pmsg != 0);
    DCHECK(pmsg->GetCode() == cc_proto::codes::kHandshakeReply);
    auto& hrmsg = pmsg->GetAs<cc_proto::HandshakeReplyMessage>();
    syslog(_INFO) << "hrmsg.GetSuccess() => " << hrmsg.GetSuccess() << "\n";

    MustSuccess(ProtoSock().WriteProtoMessage(
      cc_proto::HandshakeMessage(BotId::FromUint(13), 1, "aaa")));

    ProtoSock().Close();
  }

  // [CcServerEvents impl]
  void OnBotHandshakeComplete(Shptr<ICcBot> bot) override {
    // inside CcServerSession strand
    hshaked_botids_.push_back(bot->GetReadonlyData().GetHandshakeData()->GetBotId());
  }
  void OnBotRemovedFromList(Shptr<ICcBot> bot) override {
    // inside ccserver acceptor strand
    removed_errvecs_.push_back(bot->GetLastErrorVector());
  }
};

class UnexpectedMessageTest : public ServerProtoTester {
public:
  virtual ~UnexpectedMessageTest() = default;

  using ServerProtoTester::ServerProtoTester;

  void EnableLegitHandshake() { legit_hshake_ = true; }
  void SetMessage(Uptr<ProtoMessage> msg) { msg_ = std::move(msg); }

  void MessageIsSpoiled() {
    spoiled_ = true;
  }

  void CheckResults() {
    // Allow both connection reset and EOF errors because as I noticed,
    // at least on windows, titties tcp/ip can return EOF even when I closesocket
    // without shutdown
    ProtoError allow1 = ProtoError(ProtoErrc::stream_chunk_reader_error,
                                   ProtoErrorInfo(StreamChunkReaderError(StreamChunkReaderErrc::stream_error,
                                   StreamChunkReaderErrorInfo(boost::asio::error::eof))));
    ProtoError allow2 = ProtoError(ProtoErrc::stream_chunk_reader_error,
                                   ProtoErrorInfo(StreamChunkReaderError(StreamChunkReaderErrc::stream_error,
                                   StreamChunkReaderErrorInfo(boost::asio::error::connection_reset))));
    if (pt_read_err_ == allow1) {
      return;
    }
    if (pt_read_err_ == allow2) {
      // Note: this can hasn't been tested yet. Keep breakpoint.
      return;
    }
    DCHECK(!"unexpected last read error");
  }

private:
  bool legit_hshake_{ false };
  Uptr<ProtoMessage> msg_;
  Uptr<ProtoMessage> last_read_msg_;
  ProtoError pt_read_err_;
  bool spoiled_{false};

private:
  void DoInteraction() override {
    DCHECK(msg_);

    MustSuccess(ProtoSock().ConnectServer());

    if (legit_hshake_) {
      MustSuccess(ProtoSock().WriteProtoMessage(
        cc_proto::HandshakeMessage(BotId::FromUint(7), 6200, "aaaaa")));

      // Need to compare
      Uptr<ProtoMessage> pmsg;
      MustSuccess(ProtoSock().ReadProtoMessage(pmsg));
      DCHECK(pmsg != 0);
      DCHECK(pmsg->GetCode() == cc_proto::codes::kHandshakeReply);
      auto& hrmsg = pmsg->GetAs<cc_proto::HandshakeReplyMessage>();
      syslog(_INFO) << "LeginHandshake case: hrmsg.GetSuccess() => " << hrmsg.GetSuccess() << "\n";
    }

    // Write user-choosen message
    MustSuccess(ProtoSock().WriteProtoMessage(*msg_.get()));

    // Read message
    pt_read_err_ = ProtoSock().ReadProtoMessage(last_read_msg_);

    if (spoiled_) {
      // We've just written spoiled message, server must close connection
      Uptr<ProtoMessage> msg_unused;
      ProtoError pt_err;

      // Verify that server closed the connection
      DCHECK(pt_read_err_.GetErrc() == ProtoErrc::stream_chunk_reader_error);

      const StreamChunkReaderError& e(
            pt_read_err_.GetErrorInfo().GetStreamChunkReaderError());

      DCHECK(e.GetErrc() == StreamChunkReaderErrc::stream_error);
      DCHECK(e.GetErrorInfo().GetStreamError() == boost::asio::error::eof
             ||
             e.GetErrorInfo().GetStreamError() == boost::asio::error::connection_reset);

      //ProtoSock().Close(); // automatically closed
      syslog(_INFO) << "Spoiled msg: returning from DoInteraction()\n";
      return;
    }

    syslog(_INFO) << "Good msg: Last read error is finna be " << pt_read_err_.MakeErrorMessage() << "\n";

    ProtoSock().Close();
  }
};
}

void test_cc_proto_scenarios_srv_doublehshake(TestInfo&) {
  auto protfac(make_shared<cc_proto::MessageFactory>());
  {
    SimpleHandshakeTester pc;
    ServerProtoTesterRunner runner(pc, protfac);
    runner.Run();
  }
  {
    TwoSequentialHandshakesTester pc;
    ServerProtoTesterRunner runner(pc, protfac);
    runner.Run();
    pc.CheckResults();
  }
  {
    // handshake + handshake
    UnexpectedMessageTest pc;
    ServerProtoTesterRunner runner(pc, protfac);
    pc.EnableLegitHandshake();
    pc.SetMessage(
      make_unique<cc_proto::HandshakeMessage>(
        BotId::FromUint(7), 6200, "dahsjh"));

    runner.Run();
    pc.CheckResults();
  }
}


void test_cc_proto_scenarios_srv_badmsgs(TestInfo&) {
  auto protfac(make_shared<cc_proto::MessageFactory>());
  {
    vector<Uptr<ProtoMessage>> test_msgs;
    protfac->CreateTestMessages(test_msgs);
    for (auto& test_msg : test_msgs) {
      if (test_msg->GetCode() == cc_proto::codes::kHandshake) {
        // skip handshake, we test it separately
        continue;
      }
      UnexpectedMessageTest pc;
      ServerProtoTesterRunner runner(pc, protfac);

      // We don't pc.EnableLegitHandshake() here. So our msg goes first.
      pc.SetMessage(std::move(test_msg));

      //pc.SetMessageAfterHandshake(make_unique<cc_proto::HandshakeMessage>(
      //  BotId::FromUint(7), 6200));

      runner.Run();

      // Connection must be dropped after unexpected message from client
      pc.CheckResults();
    }
  }
}

void test_cc_proto_scenarios_srv_unknownmsgs(TestInfo&) {

  // Feed MyProtoMessage to real cc server. Must be unknown proto message condition on server.
  //auto protfac(make_shared<MyProtoMessageFactory>());

  auto protfac(make_shared<cc_proto::MessageFactory>());

  UnexpectedMessageTest test;
  ServerProtoTesterRunner runner(test, protfac);

  Uptr<ProtoMessage> msg = protfac->CreateMessageForCode(cc_proto::codes::kFire);
  // Spoil the code
  test.SetMessage(move(msg));
  test.MessageIsSpoiled();

  // can't do, no access to server session
  //DCHECK(test  ().GetErrorStack().top().GetErrc() == CcErrc::unknown_proto_message_from_client);
  //DCHECK(test. ().GetErrorStack().top().GetErrorInfo().code = 1488);

  runner.Run();

  test.CheckResults();
}








