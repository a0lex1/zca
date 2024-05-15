#include "cc/cc_client_session.h"

#include "cc/cc_proto/command_result_message.h"
#include "cc/cc_proto/command_message.h"
#include "cc/cc_proto/handshake_message.h"
#include "cc/cc_proto/handshake_reply_message.h"
#include "cc/cc_proto/ping_message.h"
#include "cc/cc_proto/fire_message.h"
#include "cc/cc_proto/skipme_message.h"

#include "co/async/wrap_post.h"
#include "co/xlog/xlog.h"

#include <atomic>

using namespace boost::posix_time;
using namespace cc::cc_proto;
using namespace std;

#define llogt() Log(_TRACE) << "CcCliSess (base)" << SPTR(static_cast<co::async::SessionBase*>(this)) << " "
#define llogd() Log(_DBG) << "CcCliSess (base)" << SPTR(static_cast<co::async::SessionBase*>(this)) << " "
#define llogw() Log(_WARN) << "CcCliSess (base)" << SPTR(static_cast<co::async::SessionBase*>(this)) << " "

namespace cc {

atomic<uint64_t> _dbgNumCcClientSessions = 0;
atomic<uint64_t> _dbgNumCcClientSessionsIoStarted = 0;
atomic<uint64_t> _dbgNumCcClientSessionsIoEnded = 0;

DEFINE_XLOGGER_SINK("ccclisess", gCcClientSessionSink);
#define XLOG_CURRENT_SINK gCcClientSessionSink

static const uint16_t g_cc_current_version_major = 0x0000;
static const uint16_t g_cc_current_version_minor = 0x0001;

CcClientSession::~CcClientSession() {
  _dbgNumCcClientSessions -= 1;
  llogt() << "~~~DTOR~~~\n";
  _dbg_dtored_ = true;
}

CcClientSession::CcClientSession(Uptr<Stream> new_stm,
                                 Shptr<Strand> strand,
                                 CcClientCommandDispatcher& cmd_disp,
                                 CcClientEvents* events,
                                 uint32_t max_chunk_body_size,
                                 Shptr<CcClientSharedData> shared_data,
                                 Uptr<string> opaque_hshake_data)
  :
  CcSession(move(new_stm), strand, max_chunk_body_size),
  cmd_disp_(cmd_disp),
  events_(events),
  hshake_postpone_timer_(make_unique<deadline_timer>(GetIoContext())),
  shared_data_(shared_data),
  ping_timer_(GetIoContext()),
  opaque_hshake_data_(move(opaque_hshake_data))
{
  llogt() << "CTOR\n";
  
  _dbgNumCcClientSessions += 1;

  SET_DEBUG_TAG(*this, "ccclisess");
  SET_DEBUG_TAG(_DbgTagOfChunkWriterQST(), string_printf("_CcCliSess_%p_chnkwrqst", static_cast<co::async::SessionBase*>(this)).c_str());
  SET_DEBUG_TAG(_DbgTagOfChunkReader(), string_printf("_CcCliSess_%p_chnkrdr", static_cast<co::async::SessionBase*>(this)).c_str());
}

void CcClientSession::StopUnsafe() {
  DCHECK(IsInsideFiberStrand());
  CcSession::StopUnsafe();
  ping_timer_.cancel();
}

void CcClientSession::BeginIo(RefTracker rt) {
  DCHECK(!_dbg_dtored_);

  rt.SetReferencedObject(shared_from_this());

  RefTracker rt_all(CUR_LOC(),
                    [&] () {
    llogt() << "; rt_all\n";
    _dbgNumCcClientSessionsIoEnded += 1;
  }, rt);

  // initiate protocol chain
  // all the dispatching is in |disp_chain_|
  _dbgNumCcClientSessionsIoStarted += 1;
  if (shared_data_->bot_opts_.GetPostponeEnable()) {
    llogd() << "postponing hshake " << shared_data_->bot_opts_.GetPostponeDelay().total_milliseconds() << " msec\n";

    hshake_postpone_timer_->expires_from_now(shared_data_->bot_opts_.GetPostponeDelay());
    hshake_postpone_timer_->async_wait(
      wrap_post(GetFiberStrand(), co::bind(&CcClientSession::HandlePostponeTimer,
      GetSharedFromThisAs<CcClientSession>(), _1, rt_all)));
  }
  else {
    //llogt() << "writing hshake w/o delay\n"; // no need to log
    WriteHandshake(rt_all);
  }
}

void CcClientSession::HandlePostponeTimer(Errcode err, RefTracker rt) {
  DCHECK(IsInsideFiberStrand());

  if (err) {
    llogd() << "HandlePostponeTimer error: " << err << " !!!\n";
    StopThreadsafe();
    return;
  }

  WriteHandshake(rt);
}

void CcClientSession::WriteHandshake(RefTracker rt) {
  DCHECK(IsInsideFiberStrand());

  llogd() << "writing HSHAKE{bot_id = " << shared_data_->bot_opts_.GetBotId().ToStringRepr() << "}\n";

  uint32_t dwVer = CO_MAKEDWORD(
    g_cc_current_version_minor,
    g_cc_current_version_major);

  const string& opaque_data = opaque_hshake_data_ != nullptr ? *opaque_hshake_data_ : "";
  auto hshake_msg(cc_proto::HandshakeMessage(shared_data_->bot_opts_.GetBotId(),
                  dwVer,
                  opaque_data));

  GetSequencedMessageWriter().AsyncWriteMessage(
    hshake_msg,
    wrap_post(GetFiberStrand(),
      co::bind(
      &CcClientSession::HandleWriteHandshake,
      GetSharedFromThisAs<CcClientSession>(), _1, rt)));
}

void CcClientSession::HandleWriteHandshake(Errcode err, RefTracker rt) {
  DCHECK(IsInsideFiberStrand());

  if (err) {
    llogd() << "HandleWriteHandshake ERROR " << err << "\n";
    CcError cc_err = CcError(CcErrc::write_failed, CcErrorInfo(err));
    PushLastError(cc_err);
    StopThreadsafe();
    return;
  }
  if (events_) {
    events_->OnClientHandshakeWritten();
  }

  llogt() << "HandleWriteHandshake: reading hshake result\n";
  ReadHandshakeResult(rt);
}

void CcClientSession::ReadHandshakeResult(RefTracker rt) {
  DCHECK(IsInsideFiberStrand());

  GetMessageReader().AsyncReadMessage(
    hshake_reply_msg_,
    wrap_post(GetFiberStrand(),
        co::bind(&CcClientSession::HandleReadHandshakeResult,
                 GetSharedFromThisAs<CcClientSession>(),
                 _1, rt)));
}

void CcClientSession::HandleReadHandshakeResult(ProtoError pt_err,
                                                RefTracker rt) {
  DCHECK(IsInsideFiberStrand());

  if (pt_err) {
    llogd() << "HandleReadHandshakeResult ERROR pt_err = " << pt_err.MakeErrorMessage() << "\n";
    DCHECK(hshake_reply_msg_ == nullptr);
    CcError cc_err = CcError(CcErrc::proto_read_error_hshake_result, CcErrorInfo(pt_err));
    PushLastError(cc_err);
    StopThreadsafe();
    return;
  }
  DCHECK(hshake_reply_msg_ != nullptr);
  if (hshake_reply_msg_->GetCode() != codes::kHandshakeReply) {
    // Message code must be kHandshakeReply. Server violates protocol.
    llogw() << "!!!server violated!!! : expecting kHandshakeReply, got other message\n";
    CcError cc_err = CcError(CcErrc::hshake_reply_expected, CcErrorInfo());
    PushLastError(cc_err);
    StopThreadsafe();
    return;
  }
  if (!hshake_reply_msg_->GetAs<cc_proto::HandshakeReplyMessage>().GetSuccess()) {
    // Message code must be kHandshakeReply. Server violates protocol.
    llogw() << "!hshake_reply_msg->GetSuccess()\n";
    CcError cc_err = CcError(CcErrc::hshake_reply_expected, CcErrorInfo());
    PushLastError(cc_err);
    StopThreadsafe();
    return;
  }

  // Server's handshake reply saved to |hshake_reply_msg_| and can be read in future
  if (events_) {
    events_->OnClientHandshakeReplyReceived();
  }

  // Execute another fiber of i/o - pinging (if enabled by server)
  // E.g. pinging is parallel to messaging
  // Decide which ping_interval_to_use_
  DCHECK(shared_data_->ping_interval_ != nullptr);
  if (!shared_data_->ping_interval_->is_zero()) {
    // Override by client
    ping_interval_to_use_ = *shared_data_->ping_interval_;
    llogd() << "using ping interval from config\n";
  }
  else {
    // Client's value is 0, let the server choose
    auto server_ping_msec = hshake_reply_msg_->GetAs<cc_proto::HandshakeReplyMessage>()
      .GetClientPingIntervalMsec();

    // If server's ping interval is zero, pinging will be disabled
    ping_interval_to_use_ = milliseconds(server_ping_msec);
    llogd() << "using ping interval from server (" << server_ping_msec << " msec)\n";
  }
  if (!ping_interval_to_use_.is_zero()) {
    SetPingTimer(rt);
  }

  // We will dispatch our messages in DispatchProtoMessage() (in derived classes)
  llogd() << "handshake result has been read, starting message loop\n";
  StartReadMessageLoop(rt);
}

void CcClientSession::SetPingTimer(RefTracker rt) {
  ping_timer_.expires_from_now(ping_interval_to_use_);
  ping_timer_.async_wait(
    wrap_post(GetFiberStrand(),
      co::bind(
      &CcClientSession::HandlePingTimer,
      GetSharedFromThisAs<CcClientSession>(), _1, rt)));
}

void CcClientSession::HandlePingTimer(Errcode err, RefTracker rt) {
  DCHECK(IsInsideFiberStrand());
  if (IsStopping() || nullptr == GetThreadsafeStopableImpl().InterlockedLoadLastRef()) {
    return;
  }
  if (!err) {
    WritePing(rt);
  }
  else {
    // If error happened, just disable the ping timer
    llogd() << "Client Pinging disabled due to error in HandlePingTimer, err = " << err << "\n";
  }
}

void CcClientSession::WritePing(RefTracker rt) {
  cc_proto::PingMessage ping_msg;

  llogd() << "Sending PING message\n";

  GetSequencedMessageWriter().AsyncWriteMessage(
    ping_msg,
    wrap_post(GetFiberStrand(),
      co::bind(
        &CcClientSession::HandleWritePingMessage,
        GetSharedFromThisAs<CcClientSession>(), _1, rt)));
}

void CcClientSession::HandleWritePingMessage(Errcode err, RefTracker rt) {
  if (!err) {
    SetPingTimer(rt);
  }
  else {
    llogd() << "Client Pinging disabled due to error in HandleWritePingMessage, err = " << err << "\n";
  }
}

// ---

void CcClientSession::DispatchProtoMessage(const ProtoMessage& msg,
                                           HandlerWithCcErr handler) {
  DCHECK(IsInsideFiberStrand());

  switch (msg.GetCode()) {
  case cc_proto::codes::kCommand: {

    // Not tested
    // We shouldn't be running command now.
    if (cur_cmd_result_opaque_data_ != nullptr) {
      // We are dispatching cmd now
      llogw() << "!!!server violated!!! : kCommand while execing another cmd\n";
      CcError cc_err = CcError(CcErrc::command_while_command, CcErrorInfo());
      handler(cc_err); // no need to push last error, handler will do it
      return;
    }
    // Dispatch command
    auto& cmd_msg(static_cast<const cc_proto::CommandMessage&>(msg));
    cur_cmd_result_opaque_data_ = make_unique<std::string>();
    //cur_cmd_result_opaque_data_.clear(); // no need, it's then moved
    cmd_disp_.DispatchCommand(
          make_unique<string>(cmd_msg.GetOpaqueData()),
            *cur_cmd_result_opaque_data_.get(),
            wrap_post(GetFiberStrand(),
                      co::bind(&CcClientSession::HandleDispatchCommand,
                                  GetSharedFromThisAs<CcClientSession>())));

    // continue proto msg dispatching
    handler(CcError::NoError());
    break;
  }
  default: {
    llogw() << " ***UNKNOWN PROTO MSG FROM SERVER [code " << msg.GetCode() << "], stopping*** \n";

    CcError cc_err = CcError(CcErrc::unknown_proto_message_from_server, CcErrorInfo());
    handler(cc_err); // no need to push last error, handler will do it
    break;
  }
  }
}

void CcClientSession::HandleDispatchCommand() {
  DCHECK(IsInsideFiberStrand());
  DCHECK(cur_cmd_result_opaque_data_);

  cc_proto::CommandResultMessage cmd_result_msg(cur_seq_num_++,
                                                std::move(cur_cmd_result_opaque_data_));
  GetSequencedMessageWriter().AsyncWriteMessage(
    cmd_result_msg,
    wrap_post(GetFiberStrand(),
        co::bind(&CcClientSession::HandleWriteCommandResult,
                 GetSharedFromThisAs<CcClientSession>(), _1)));
}

void CcClientSession::HandleWriteCommandResult(Errcode err) {
  DCHECK(IsInsideFiberStrand());

  DCHECK(!cur_cmd_result_opaque_data_); // moved

  if (shared_data_->bot_opts_._IsInjectHandleWriteCommandResultErrorEnabled()) {
    // Simulate error
    PushLastError(CcError(CcErrc::transport_error_hwcr));
    StopThreadsafe();
    return;
  }

  if (err) {
    PushLastError(CcError(CcErrc::transport_error_hwcr));
    StopThreadsafe();
    return;
  }

  // Nothing to do. We're already reading new commands.
}

}

