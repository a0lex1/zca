#include "cc/cc_server_session.h"
#include "cc/cc_server.h"
#include "cc/find_bot_in_list.h"

#include "cc/cc_proto/handshake_reply_message.h"
#include "cc/cc_proto/handshake_message.h"

#include "co/async/wrap_post.h"
#include "co/base/rand_gen.h"
#include "co/xlog/xlog.h"

#include <boost/make_shared.hpp>

using namespace co;
using namespace std;
using namespace boost::asio;
using namespace boost::posix_time;

#define llogt() Log(_TRACE) << "CcServSess (base)" << SPTR(static_cast<co::async::SessionBase*>(this)) << " "
#define llogd() Log(_DBG) << "CcServSess (base)" << SPTR(static_cast<co::async::SessionBase*>(this)) << " "
#define llogw() Log(_WARN) << "CcServSess (base)" << SPTR(static_cast<co::async::SessionBase*>(this)) << " "

namespace cc {

atomic<uint64_t> _dbgNumCcServerSessions = 0;
atomic<uint64_t> _dbgNumCcServerSessionsIoStarted = 0;
atomic<uint64_t> _dbgNumCcServerSessionsIoEnded = 0;

DEFINE_XLOGGER_SINK("ccservsess", gCcServerSessionSink);
#define XLOG_CURRENT_SINK gCcServerSessionSink

CcServerSession::~CcServerSession() {
  _dbgNumCcServerSessions -= 1;
  llogt() << "~~~DTOR~~~\n";
  _dbg_dtored_ = true;
}

CcServerSession::CcServerSession(Uptr<Stream> new_stm,
                                 Shptr<Strand> strand,
                                 uint32_t max_chunk_body_size,
                                 CcServerEvents* ev_disp,
                                 time_duration ping_interval)
  :
  CcSession(std::move(new_stm), strand, max_chunk_body_size),
  ev_disp_(ev_disp),
  readonly_data_(make_unique<BotReadonlyData>()),
  ping_interval_(ping_interval)
{
  llogt() << "CTOR\n";

  _dbgNumCcServerSessions += 1;

  DCHECK(nullptr == readonly_data_->GetHandshakeData());

  SET_DEBUG_TAG(*this, "ccservsess");
  SET_DEBUG_TAG(_DbgTagOfChunkWriterQST(), string_printf("_CcServSess_%p_chnkwrqst", static_cast<co::async::SessionBase*>(this)).c_str());
  SET_DEBUG_TAG(_DbgTagOfChunkReader(), string_printf("_CcServSess_%p_chnkrdr", static_cast<co::async::SessionBase*>(this)).c_str());
}

void CcServerSession::BeginIo(RefTracker rt) {

  DCHECK(!_dbg_dtored_);
  DCHECK(owner_server_ != 0);

  rt.SetReferencedObject(shared_from_this());

  llogd() << "BeginIo\n";
  _dbgNumCcServerSessionsIoStarted += 1;

  auto sh_this = GetSharedFromThisAs<CcServerSession>();
  RefTracker rt_all(CUR_LOC(),
                    [&, sh_this] () {
    _dbgNumCcServerSessionsIoEnded += 1;
    llogd() << "; rt_all\n";
    (void)sh_this;
                     },
                     rt);

  Errcode err;
  GetStream().GetRemoteAddress(readonly_data_->remote_addr_, err);
  if (err) {
    NOTREACHED();
  }

  StartReadMessageLoop(rt_all);
}

// [ICcBot impl]

void CcServerSession::Kill() {
  llogd() << "Kill: doing StopThreadsafe\n";
  StopThreadsafe();
}

ICcBotReadonlyData& CcServerSession::GetReadonlyData() {
  return *readonly_data_.get();
}

void CcServerSession::SetUserData(BoostShptr<CcUserData> user_data) {
  user_data_atomic_.store(user_data);
}

BoostShptr<CcUserData> CcServerSession::GetUserData() {
  return user_data_atomic_.load();
}

void CcServerSession::ExecuteSequencedCommand(Uptr<string> cmd_opaque_data,
                                              string& cmd_result_opaque_data,
                                              HandlerWithErrcode handler) {
  Shptr<string> cmd_opaque_data_shptr(std::move(cmd_opaque_data));

  // LOCATION STACKTRACE

  boost::asio::dispatch(
    GetFiberStrand(),
    co::bind(&CcServerSession::ExecuteSequencedCommandUnsafe,
      GetSharedFromThisAs<CcServerSession>(),
      cmd_opaque_data_shptr,
      std::ref(cmd_result_opaque_data),
      handler));
}

void CcServerSession::ExecuteSequencedCommandUnsafe(Shptr<string> cmd_opaque_data,
                                                    string& cmd_result_opaque_data,
                                                    HandlerWithErrcode handler) {
  DCHECK(IsInsideFiberStrand());
  DCHECK(handler);

  Shptr<RefTracker> _lastref_sh;
  _lastref_sh = GetThreadsafeStopableImpl().InterlockedLoadLastRef();

  if (_lastref_sh == nullptr) {
    // We were stopped during post() of ExecuteSequencedCommandUnsafe
    DCHECK(!user_seqcmd_handler_); // must be deleted in StopUnsafe < bug disappeared if uncomment
    llogd() << "our_last_ref_sh == nullptr! (stopped during post)\n";
    boost::asio::post(GetIoContext(), [handler]() {
      handler(boost::asio::error::operation_aborted);
                        });
    return;
  }

  if (IsStopping()) {
    llogd() << "IsStopping() ! (stopped during post)\n";
    boost::asio::post(GetIoContext(), [handler]() {
      handler(boost::asio::error::operation_aborted);
                        });
    return;
  }

  // Inject
  RefTracker rt_lastref = *_lastref_sh.get();
  _lastref_sh = nullptr;

  if (bools_.doing_seq_cmd_) {
    boost::asio::post(GetIoContext(), [handler, rt_lastref]() {
      // Don't call directly. Idk wtf can go wrong.
      handler(boost::asio::error::already_started);
      (void)rt_lastref; // #CarryingKeepAlive
                        });
    return;
  }

  DCHECK(!user_seqcmd_handler_);
  DCHECK(cmd_result_opaque_data_ == nullptr);

  int calculated_sig = 0x1337228;

  bools_.doing_seq_cmd_ = true;
  user_seqcmd_handler_ = handler;
  cmd_result_opaque_data_ = &cmd_result_opaque_data;

  seq_cmd_msg_ = make_unique<cc_proto::CommandMessage>(cur_seq_num_++,
                                                       calculated_sig,
                                                       cmd_opaque_data);
  // Need to pass our last ref through the io fiber (chain)
  GetSequencedMessageWriter()
    .AsyncWriteMessage(*seq_cmd_msg_.get(),
                       wrap_post(GetFiberStrand(),
                         co::bind(
                         &CcServerSession::HandleWriteSeqCommandMessage,
                           GetSharedFromThisAs<CcServerSession>(),
                           _1, rt_lastref)));
}

void CcServerSession::HandleWriteSeqCommandMessage(Errcode err,
                                                   RefTracker our_last_ref) {
  DCHECK(IsInsideFiberStrand());
  seq_cmd_msg_ = nullptr;

  if (err) {
    llogd() << "HandleWriteSeqCommandMessage err " << err << "\n";

    // Abort seqcmd, we will not process CommandResult from peer
    bools_.doing_seq_cmd_ = false;
    // Don't call the handler. Is it OK?
    user_seqcmd_handler_ = nullptr;
    cmd_result_opaque_data_ = nullptr;

    PushLastError(CcError(CcErrc::transport_error_hwscm, CcErrorInfo(err)));
    StopThreadsafe();
  }

  // |our_last_ref| is going to be released
}


// [CcSession (ProtoMessageDispatcher) impl]

void CcServerSession::DispatchProtoMessage(const ProtoMessage& msg,
                                           HandlerWithCcErr handler)
{
  DCHECK(IsInsideFiberStrand());

  if (nullptr == GetReadonlyData().GetHandshakeData()) {
    // The bot hasn't handshaked with us yet.
    // The only available message for him is kHandshake.
    if (msg.GetCode() != cc_proto::codes::kHandshake) {
      llogw() << "!!!bot_violated!!!: non-handshake message before handshake (code " << msg.GetCode() << ")\n";
      handler(CcError(CcErrc::unknown_proto_message_from_client, CcErrorInfo(msg.GetCode())));
      return;
    }
  }

  using HandshakeMessage = cc_proto::HandshakeMessage;

  switch (msg.GetCode()) {
  case cc_proto::codes::kHandshake:
    DispatchHandshakeMessage(msg.GetAs<const cc_proto::HandshakeMessage>(), handler);
    break;
  case cc_proto::codes::kPing:
    llogd() << "Ping message from bot\n";
    DispatchPingMessage(msg.GetAs<const cc_proto::PingMessage>(), handler);
    break;
  case cc_proto::codes::kCommandResult:
    llogd() << "CommandResult message from bot\n";
    DispatchCommandResultMessage(msg.GetAs<const cc_proto::CommandResultMessage>(), handler);
    break;
  default:
    llogw() << "!!!bot_violated!!!: unknown message (code " << msg.GetCode() << ")\n";
    handler(CcError(CcErrc::unknown_proto_message_from_client, CcErrorInfo(msg.GetCode())));
    break;
  }
}

// Hook StopUnsafe. Deinitialize our fields, then pass to CcSession::StopUnsafe().
void CcServerSession::StopUnsafe() {
  llogd() << "StopUnsafe\n";

  //bools_.stop_unsafe_called_ = true;

  // If we're now waiting for seq cmd result, drop handler by clearing queue
  user_seqcmd_handler_ = nullptr;

  CcSession::StopUnsafe();
}

void CcServerSession::StopThreadsafe() {
  // just wanted to log this func
  llogd() << "StopThreadsafe\n";
  CcSession::StopThreadsafe();
}

void CcServerSession::DispatchCommandResultMessage(
  const cc_proto::CommandResultMessage& cmd_result_msg,
  HandlerWithCcErr handler)
{
  DCHECK(IsInsideFiberStrand());

  if (!bools_.doing_seq_cmd_) {
    DCHECK(!user_seqcmd_handler_);
    llogw() << "!!!bot_violated!!!: Got CommandResult message, but we're not |doing_executed_seq_cmd_|\n";
    handler(CcError(CcErrc::unexpected_command_result,
            CcErrorInfo(cmd_result_msg.GetCode())));
    return;
  }
  if (!user_seqcmd_handler_) {
    // We were already stopped our CcServerSession
    handler(CcError(CcErrc::seqcmd_aborted));
    return;
  }
  DCHECK(cmd_result_opaque_data_ != nullptr);

  // All fine, bot has responded to cmd we sent. End up command transaction
  bools_.doing_seq_cmd_ = false;

  // Complete user's ExecuteSequencedCmd() operation.
  *cmd_result_opaque_data_ = cmd_result_msg.GetOpaqueData();
  cmd_result_opaque_data_ = nullptr; // clear our ptr field
  user_seqcmd_handler_(NoError());
  user_seqcmd_handler_ = nullptr;

  // Return to the CcSession's msg read loop.
  // Safe to call handler() directly, we're here from ProtoMessage dispatcher.
  handler(CcError::NoError());
}

void CcServerSession::HandshakeAuxBotListCallback(ICcBotList& bl,
                                                  cc_proto::HandshakeMessage hshake_msg,
                                                  HandlerWithCcErr return_to) {
  // INSIDE CC SERVER BOTLIST FIBER
  // We are keeping |hshake_msg| a copy intentionally.
  DCHECK(owner_server_->IsInsideAcceptorStrand());
  bools_.hshake_in_progress_ = false;

  // Ensure bot id doesn't already exist
  const BotId bot_id(hshake_msg.GetBotId());
  Shptr<ICcBot> bot = FindBotInList(bl, bot_id);
  if (bot) {
    // ErrorID = Violation
    // Bot with this ID is already in list
    llogd() << "Handshake: Bot ID is ALREADY IN USE! botid=" << bot_id.ToStringRepr() << "\n";
    return_to(CcError(CcErrc::bot_id_in_use, CcErrorInfo(hshake_msg.GetCode())));
    return;
  }
  // OK, this bot id is new
  llogd() << "Got HANDSHAKE on server side, bot id " << bot_id.ToStringRepr() << "\n";

  // Initialize BotHandshakeData before initiating write i/o
  readonly_data_->hshake_data_ = make_unique<BotHandshakeData>(hshake_msg);
  // Set first ping time to handshake time
  readonly_data_->last_ping_time_ = boost::posix_time::second_clock::local_time();

  //
  // Return from botlist fiber to session fiber
  //
  return_to(CcError::NoError());
}

void CcServerSession::DispatchHandshakeMessage(
  const cc_proto::HandshakeMessage& hshake_msg, HandlerWithCcErr handler) // #CarryingHandler
{
  if (bools_.hshake_in_progress_) {
    llogw() << "!!!bot_violated!!!: handshake during handshake!\n";
    handler(CcError(CcErrc::handshake_during_handshake,
            CcErrorInfo(hshake_msg.GetCode())));
    return;
  }
  // Ensure this bot hasn't handshaked yet
  if (readonly_data_->hshake_data_ != nullptr) {
    llogw() << "!!!bot_violated!!!: second handshake\n";
    handler(CcError(CcErrc::second_handshake,
            CcErrorInfo(hshake_msg.GetCode())));
    return;
  }

  llogd() << "DispatchHandshakeMessage: doing ExecuteBotListAccessCallback";

  bools_.hshake_in_progress_ = true;

  auto return_to = co::async::wrap_post(
        GetFiberStrand(),
        co::bind(&CcServerSession::HandshakeContinue,
                 GetSharedFromThisAs<CcServerSession>(),
                 _1, hshake_msg, handler));

  // Execute callback that finds bot with this is and if it doesn't exist, allows it
  owner_server_->ExecuteBotListAccessCallback(
        co::bind(
          &CcServerSession::HandshakeAuxBotListCallback,
          GetSharedFromThisAs<CcServerSession>(),
          _1, std::ref(hshake_msg), return_to));
}

void CcServerSession::DispatchPingMessage(
  const cc_proto::PingMessage& ping_msg, HandlerWithCcErr handler) // #CarryingHandler
{
  DCHECK(IsInsideFiberStrand());
  
  // Update the current ping time
  readonly_data_->last_ping_time_ = boost::posix_time::second_clock::local_time();

  handler(CcError::NoError());
}

void CcServerSession::HandshakeContinue(const CcError& cc_err,
                                        const cc_proto::HandshakeMessage& hshake_msg,
                                        HandlerWithCcErr handler) { // #CarryingHandler
  DCHECK(IsInsideFiberStrand());
  // |cc_err| is from HandshakeAuxBotListCallback. If it's not OK, hshake failed.
  if (cc_err) {
    handler(cc_err);
    return;
  }
  if (IsStopping()) {
    llogd() << "HandshakeContinue? No, IsStopping\n";
    handler(CcError(CcErrc::hshake_aborted));
    return;
  }
  // Write reply. We can capture ref to |msg| because it's alive until we call |handler|
  // OnBotHandshakeComplete() will be called if handshake reply written successfully
  auto write_handler(
        co::bind(&CcServerSession::HandleWriteHandshakeReply,
                 GetSharedFromThisAs<CcServerSession>(),
                 _1, std::ref(hshake_msg), handler));

  GetSequencedMessageWriter().AsyncWriteMessage(
    cc_proto::HandshakeReplyMessage(true, ping_interval_.total_milliseconds()),
    wrap_post(GetFiberStrand(), write_handler));
}


void CcServerSession::HandleWriteHandshakeReply(Errcode err,
                                                const cc_proto::HandshakeMessage& hshake_msg,
                                                HandlerWithCcErr handler) { // #CarryingHandler
  DCHECK(IsInsideFiberStrand());
  if (err) {
    handler(CcError(CcErrc::transport_error_hwhr, CcErrorInfo(err)));
    return;
  }
  if (ev_disp_) {
    // Notify user's event dispatcher
    // We need ICcBot interface, we are implementing it
    // So use GetSharedFromThisAs
    ev_disp_->OnBotHandshakeComplete(GetSharedFromThisAs<CcServerSession>());
  }
  // Continue execution (return to CcSession)
  handler(CcError::NoError());
}


}

