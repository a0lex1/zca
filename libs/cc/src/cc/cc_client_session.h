#pragma once

#include "cc/cc_client_shared_data.h"
#include "cc/cc_client_bot_options.h"
#include "cc/cc_client_command_dispatcher.h"
#include "cc/cc_client_events.h"
#include "cc/cc_session.h"

#include <boost/asio/deadline_timer.hpp>

namespace cc {

DECLARE_XLOGGER_SINK("ccclisess", gCcClientSessionSink);

extern std::atomic<uint64_t> _dbgNumCcClientSessions;
extern std::atomic<uint64_t> _dbgNumCcClientSessionsIoStarted;
extern std::atomic<uint64_t> _dbgNumCcClientSessionsIoEnded;

class CcClientSession : public CcSession {
public:
  virtual ~CcClientSession();

  using time_duration = boost::posix_time::time_duration;
  using deadline_timer = boost::asio::deadline_timer;

  // |strand| is passed to Session as usual
  CcClientSession(Uptr<Stream> new_stm,
                  Shptr<Strand> strand,
                  CcClientCommandDispatcher& cmd_disp,
                  CcClientEvents* events,
                  uint32_t max_chunk_body_size,
                  Shptr<CcClientSharedData> shared_data,
                  Uptr<std::string> opaque_hshake_data);

protected:
  auto shared_from_this() { return co::shared_from(this); }

private:
  using StreamChunkWriterQueueST = co::async::StreamChunkWriterQueueST;

  void BeginIo(RefTracker rt) override;
  void StopUnsafe() override;

  void WriteHandshake(RefTracker);
  void HandleWriteHandshake(Errcode, RefTracker);
  void ReadHandshakeResult(RefTracker);
  void HandleReadHandshakeResult(ProtoError, RefTracker);
  void HandlePostponeTimer(Errcode, RefTracker);
  
  // [CcSession::ProtoMessageDispatcher impl]
  void DispatchProtoMessage(const ProtoMessage&, HandlerWithCcErr) override;

  void HandleDispatchCommand();
  void HandleWriteCommandResult(Errcode);

  std::vector<CcError>& GetLastErrorStack() override { return shared_data_->err_stack_; }

  // pinging routines
  void SetPingTimer(RefTracker);
  void HandlePingTimer(Errcode, RefTracker);
  void WritePing(RefTracker);
  void HandleWritePingMessage(Errcode, RefTracker);

private:
  Shptr<CcClientSharedData> shared_data_;
  Uptr<std::string> opaque_hshake_data_;
  CcClientCommandDispatcher& cmd_disp_;
  CcClientEvents* events_;
  Uptr<ProtoMessage> hshake_reply_msg_;
  Uptr<deadline_timer> hshake_postpone_timer_;
  uint32_t cur_seq_num_{ 0 };
  Uptr<std::string> cur_cmd_result_opaque_data_; // if not nullptr, we are dispatching cmd now

  time_duration ping_interval_to_use_;
  boost::asio::deadline_timer ping_timer_;
  bool _dbg_dtored_{false};
};

}

