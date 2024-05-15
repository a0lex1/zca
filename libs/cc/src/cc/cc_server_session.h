#pragma once

#include "cc/bot_readonly_data.h"

#include "cc/cc_session.h"
#include "cc/cc_bot.h"
#include "cc/cc_bot_list.h"
#include "cc/cc_server_events.h"

#include "cc/cc_proto/command_message.h"
#include "cc/cc_proto/command_result_message.h"
#include "cc/cc_proto/ping_message.h"
#include "cc/cc_proto/codes.h"

#include <boost/asio/strand.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <atomic>

namespace cc {
  
DECLARE_XLOGGER_SINK("ccservsess", gCcServerSessionSink);

extern std::atomic<uint64_t> _dbgNumCcServerSessions;
extern std::atomic<uint64_t> _dbgNumCcServerSessionsIoStarted;
extern std::atomic<uint64_t> _dbgNumCcServerSessionsIoEnded;


/* class ServerSession
* 
* Single Threaded (strand)
* CcServerSession is already enable_shared_from_this<> (it's inside CcSession)
*/
class CcServerSession :
  public CcSession,
  public ICcBot
{
public:
  virtual ~CcServerSession();
  
  using ptime = boost::posix_time::ptime;
  using time_duration = boost::posix_time::time_duration;

  CcServerSession(Uptr<Stream> new_stm,
                  Shptr<Strand> strand,
                  uint32_t max_chunk_body_size,
                  CcServerEvents* ev_disp,
                  time_duration ping_interval);

  void SetOwnerServer(CcServer& owner_server) { owner_server_ = &owner_server; }

protected:
  auto shared_from_this() { return co::shared_from(this); }

private:
  void BeginIo(RefTracker rt) override;
  void StopUnsafe() override;
  void StopThreadsafe() override; // wanna log

  // [ICcBot impl]
  void Kill() override;
  ICcBotReadonlyData& GetReadonlyData() override;
  void SetUserData(BoostShptr<CcUserData> user_data) override;
  BoostShptr<CcUserData> GetUserData() override;
  // If another sequenced cmd already running, calls handler with error already_started
  void ExecuteSequencedCommand(Uptr<std::string> cmd_opaque_data,
                               std::string& cmd_result_opaque_data,
                               HandlerWithErrcode handler) override;

  // [CcSession::ProtoMessageDispatcher impl]
  void DispatchProtoMessage(const ProtoMessage&, HandlerWithCcErr) override;


  std::vector<CcError>& GetLastErrorStack() override {
    // Impl for CcSession
    return err_stack_;
  }

  const std::vector<CcError>& GetLastErrorVector() const override {
    // A "bridge" for ICcBot interface
    return err_stack_;
  }

private:
  // internal
  void HandleWriteHandshakeReply(Errcode, const cc_proto::HandshakeMessage&, HandlerWithCcErr);
  void HandleWriteSeqCommandMessage(Errcode, RefTracker our_last_ref);
  void HandshakeAuxBotListCallback(ICcBotList&, cc_proto::HandshakeMessage, HandlerWithCcErr);
  void HandshakeContinue(const CcError&, const cc_proto::HandshakeMessage&, HandlerWithCcErr);

  void DispatchHandshakeMessage(const cc_proto::HandshakeMessage&, HandlerWithCcErr);
  void DispatchCommandResultMessage(const cc_proto::CommandResultMessage&, HandlerWithCcErr);
  void DispatchPingMessage(const cc_proto::PingMessage&, HandlerWithCcErr);

  void ExecuteSequencedCommandUnsafe(Shptr<std::string> cmd_opaque_data,
                             std::string& cmd_result_opaque_data,
                             HandlerWithErrcode handler);

  /*void _DoBackgroundCheck() { // INSIDE ACCEPTOR FIBER StopIfZombie(); }
  void StopIfZombie() { auto time_now(boost::posix_time::second_clock::local_time()); if (time_now - *last_recv_time_.load() > ping_interval_) { CcSession::Stop(); } }*/
  //void DoBackgroundCheck;

private:
  struct {
    bool doing_seq_cmd_ : 1;
    bool hshake_in_progress_ : 1;
    //bool stop_unsafe_called_ : 1;

  } bools_{ false, false, /*false*/ };

  std::vector<CcError> err_stack_;
  CcServer* owner_server_{ nullptr };
  CcServerEvents* ev_disp_;
  Uptr<BotReadonlyData> readonly_data_;
  std::string* cmd_result_opaque_data_{ nullptr };
  HandlerWithErrcode user_seqcmd_handler_;
  Uptr<ProtoMessage> seq_cmd_msg_;
  uint32_t cur_seq_num_{ 0 };
  AtomicShptr<CcUserData> user_data_atomic_;
  time_duration ping_interval_;

  bool _dbg_dtored_{false};
};

}
