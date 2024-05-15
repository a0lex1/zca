#include "./adapter.h"
#include "./emergency.h"
#include "./factories.h"
#include "./handle_context.h"

#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;

#define EI EmergencyInfo

#define llog() \
  syslog(_DBG) << "VlAdap " << SPTR(this) \
               << " `" << GET_DEBUG_TAG(DebugTag()) << "` " << std::dec

VlanAdapter::VlanAdapter(Shptr<Strand> strand,
                         const VlanAdapterParams& adap_params)
  :
  kconfig_{
    adap_params.max_accept_handles,
    adap_params.queue_size,
    adap_params.read_buffer_size
  },
  strand_(strand),
  adap_params_(adap_params),
  accepted_handles_(adap_params.max_accept_handles)
{
  llog() << "CTOR \n";
}

void VlanAdapter::SwitchOffThreadsafe() {
  // atomic
  switching_off_ = true;

/////////////// SUKA THRAEDSAFE STOPABLE = STRAND EVERYWHERE /////////////////
  boost::asio::dispatch(*strand_.get(), co::bind(&VlanAdapter::SwitchOffUnsafe, this));
}

VlanAdapter::~VlanAdapter() {
  llog() << "~~~DTOR ~~~\n";
}

// [VlanNativeApi impl]

void VlanAdapter::VnReserveAddressTSafe(const VlanEndpoint& ep, VlanError& vlerr) {
  // r/w lock
}

void VlanAdapter::VnReleaseAddressTSafe(const VlanEndpoint& ep) {
  // r/w lock
}

void VlanAdapter::VnAccept(vlhandle_t& new_handle, const VlanEndpoint& locaddr,
                          HandlerWithVlErr uhandler, io_context& ioc_cbk)
{
  boost::asio::dispatch(*strand_.get(), co::bind(&VlanAdapter::VnAcceptUnsafe,
                        this, std::ref(new_handle), locaddr, uhandler, std::ref(ioc_cbk)));
}

void VlanAdapter::VnAcceptUnsafe(vlhandle_t& new_handle, VlanEndpoint locaddr,
                                 HandlerWithVlErr uhandler, io_context& ioc_cbk) {
  DCHECK(InsideStrand());
  if (DealWithSwitchedOff(uhandler, ioc_cbk)) {
    return;
  }
  auto it = current_accepts_.find(locaddr);
  if (it != current_accepts_.end()) {
    // this endpoint is already bound. repost, prevent evil reenter
    ioc_cbk.post([=]() {
      uhandler(VlanError(VlanErrc::address_already_bound));
                 });
    return;
  }
  // Insert the Entry
  current_accepts_[locaddr] = AcceptConnectData{ &new_handle, uhandler, &ioc_cbk };
}

void VlanAdapter::VnConnect(vlhandle_t& uhandle, const VlanEndpoint& ep,
                            HandlerWithVlErr handler, io_context& ioc_cbk)
{
  if (DealWithSwitchedOff(handler, ioc_cbk)) {
    return;
  }
  boost::asio::post(*strand_.get(), co::bind(&VlanAdapter::VnConnectUnsafe,
                    this, std::ref(uhandle), ep, handler, std::ref(ioc_cbk)));
}

void VlanAdapter::VnConnectUnsafe(vlhandle_t& new_handle, VlanEndpoint ep/*COPIED*/,
                                  HandlerWithVlErr uhandler, io_context& ioc_cbk) {
  DCHECK(InsideStrand());
  if (DealWithSwitchedOff(uhandler, ioc_cbk)) {
    return;
  }
  if (peer_kconfig_ != nullptr) {
    // Peer's kconfig already seen, check peer's accept limit
    DCHECK(connected_handles_ != nullptr);
    if (!connected_handles_->CanCreateHandle()) {
      boost::asio::post(ioc_cbk, [uhandler]() {
        uhandler(VlanError(VlanErrc::peer_accept_limit_reached));
                        });
      return;
    }
  }
  Uptr<VlanKernelConfig> attached_kconfig;
  if (!kconfig_sent_) {
    // Attach our kconfig only first connect message.
    attached_kconfig = make_unique<VlanKernelConfig>(kconfig_);
    kconfig_sent_ = true;
  }
  connect_queue_.push_back(AcceptConnectData{ &new_handle, uhandler, &ioc_cbk });
  //local_connects_[new_handle].SetState(WAITING_CONNECT_RESULT);
  PushFrame<VlanConnectMessage>(ep, move(attached_kconfig));
}

void VlanAdapter::VnRead(vlhandle_t handle, mutable_buffers_1 buf,
                         HandlerWithVlErrSize handler, io_context& ioc_cbk)
{
  boost::asio::dispatch(*strand_.get(), co::bind(&VlanAdapter::VnReadUnsafe,
                        this,
                        handle, buf, handler, std::ref(ioc_cbk)));
}

void VlanAdapter::VnReadUnsafe(vlhandle_t handle, mutable_buffers_1 buf,
                               HandlerWithVlErrSize uhandler, io_context& ioc_cbk)
{
  DCHECK(InsideStrand());
  auto handle_ctx = FindHandleContext(handle);
  DCHECK(handle_ctx != nullptr);
  // Disallow parallel reads
  DCHECK(!handle_ctx->IsUserReadSet());

  size_t avail = handle_ctx->NumAvailableForRead();
  if (avail != 0) {
    DCHECK(handle_ctx->NumAvailableForRead() <= kconfig_.buffer_size);

    // Data available for read right now
    size_t bytes_consumed;
    handle_ctx->ConsumeReadData(buf, bytes_consumed);

    DCHECK(avail - handle_ctx->NumAvailableForRead() > 0); // at least 1 consumed
    DCHECK(bytes_consumed <= avail);

    boost::asio::post(ioc_cbk, [=]() {
      uhandler(VlanError::NoError(), bytes_consumed);
                      });
  }
  else {
    // No data available for read, schedule read
    handle_ctx->SetUserRead(buf, uhandler);
  }
}

void VlanAdapter::VnWrite(vlhandle_t handle, const_buffers_1 buf,
                          HandlerWithVlErrSize handler)
{
  boost::asio::dispatch(*strand_.get(), co::bind(&VlanAdapter::VnWriteUnsafe,
                        this,
                        handle, buf, handler));
}

void VlanAdapter::VnWriteUnsafe(vlhandle_t handle, const_buffers_1 buf,
                                HandlerWithVlErrSize uhandler)
{
  DCHECK(InsideStrand());
  auto hctx = FindHandleContext(handle);
  if (!hctx) {
    // Handle was closed
    uhandler(VlanError(VlanErrc::aborted), 0);
    return;
  }

  //if (handle_data->nwrite_until_wait_drain) {
  //}
  // Can be parallel writes. Writes are sent together with any
  // other fcking messages, through a write queue. Here we
  // just remember how many fcking writes we have.
  hctx->IncrementNumWritesInProgress();

  auto buf_copy(make_unique<string>(static_cast<const char*>(
    buf.data()),
    buf.size()));

  PushFrameWithCbk<VlanDataMessage>(co::bind(&VlanAdapter::HandleWriteDataMessage,
                                    this,
                                    _1,
                                    hctx,
                                    uhandler,
                                    buf.size()),
                                handle,
                                move(buf_copy));
}

void VlanAdapter::HandleWriteDataMessage(const VlanError& vle,
                                         Shptr<HandleContext> hctx,
                                         HandlerWithVlErrSize uhandler,
                                         size_t write_size) {
  DCHECK(InsideStrand());
  if (vle) {
    // This error comes from VlanTransport::AsyncWriteFrame
    syslog(_DBG) << "aborting: VlanError " << vle.MakeErrorMessage() << "\n";
    return;
  }
  /*if (hctx->AtomicIsClosed()) {
    // closed during posting to fiber
    syslog(_DBG) << "handle was closed during posting\n";
    uhandler(vle, 0);
    return;

  }
*/
  DCHECK(hctx->GetNumWritesInProgress() > 0);

  hctx->DecrementNumWritesInProgress();

  // Direct call, user can reenter.
  uhandler(vle, write_size);
}


void VlanAdapter::VnCancelAccept(vlhandle_t handle, VlanError& vlerr)
{
}

void VlanAdapter::VnShutdownSend(vlhandle_t handle, VlanError& vlerr)
{
  boost::asio::dispatch(*strand_.get(),
                        co::bind(&VlanAdapter::VnShutdownSendUnsafe,
                        this, handle));
  vlerr = VlanError::NoError();
}

void VlanAdapter::VnShutdownSendUnsafe(vlhandle_t handle) {
  DCHECK(InsideStrand());
  auto hctx = FindHandleContext(handle);
  if (!hctx) {
    RaiseEmergency();
    return;
  }
  if (hctx->IsShutdownSent()) {
    return;
  }
  hctx->SetShutdownSent();
  PushFrameWithCbk<VlanEofMessage>(co::bind(&VlanAdapter::HandleWriteEofMessage,
                                    this,
                                    _1,
                                    hctx,
                                    uhandler,
                                    buf.size()),
                                    handle,
                                    move(buf_copy));
}

void VlanAdapter::VnClose(vlhandle_t handle)
{
}

Shptr<HandleContext> VlanAdapter::FindHandleContext(vlhandle_t h) {
  Shptr<HandleContext>* pctx;
  // Look in accepted handles
  pctx = accepted_handles_.FindHandleData(h);
  if (pctx != nullptr) {
    return *pctx;
  }
  // We can also look in connected handles if it's already created
  if (connected_handles_) {
    DCHECK(peer_kconfig_);
    pctx = connected_handles_->FindHandleData(h);
    if (pctx != nullptr) {
      return *pctx;
    }
  }
  else {
    DCHECK(!peer_kconfig_);
  }
  return nullptr;
}

// -------------------------------------------------------------------------------------------

// [FrameHandler impl]

void VlanAdapter::InputFrame(const Frame& frame)
{
  DCHECK(InsideStrand());
  if (switching_off_) {
    return;
  }
  // called from transport
  DCHECK(frame_writer_);
  switch (frame.GetProtoMessage().GetCode()) {
  case vlan_msg_codes::kVlanConnect:
    OnConnectFrameRead(frame.AsConnect());
    break;
  case vlan_msg_codes::kVlanConnectResult:
    OnConnectResultFrameRead(frame.AsConnectResult());
    break;
  case vlan_msg_codes::kVlanEmergency:
    OnEmergencyFrameRead(frame.AsEmergency());
    break;
  case vlan_msg_codes::kVlanData:
    OnDataFrameRead(frame.AsData());
    break;
  default:
    // TODO: error_fn_  and test it!
    break;
  }
}

void VlanAdapter::TransferBufferData(mutable_buffers_1 dstbuf, const_buffers_1 srcbuf,
                                     size_t& copied) {
  copied = srcbuf.size();
  if (copied > dstbuf.size()) {
    copied = dstbuf.size();
  }
  memcpy(dstbuf.data(), srcbuf.data(), copied);
}

void VlanAdapter::OnDataFrameRead(const VlanDataMessage& datamsg) {
  vlhandle_t vlhandle = datamsg.GetVlHandle();
  Shptr<HandleContext> hctx = FindHandleContext(vlhandle);
  if (hctx == nullptr) {
    RaiseEmergency(EI(EmergencyReason::protocol_violated,
                   ViolationTag_KConfigExpectedConnect));
    return;
  }
  if (hctx->IsUserReadSet()) {
    // Complete user's read

    HandlerWithVlErrSize uhandler;
    size_t bytes_transferred;

    TransferBufferData(hctx->GetUserReadBuffer(),
                       datamsg.ConstBuffer(),
                       bytes_transferred);

    uhandler = hctx->GetUserReadHandler();

    // Clear and call handler directly
    hctx->UnsetUserRead();

    uhandler(VlanError::NoError(), bytes_transferred);

    DCHECK(!hctx->IsUserReadSet());
  }
  else {
    // User is not reading, add to buffer
    if (!hctx->CommitReadData(datamsg.ConstBuffer())) {
      //!!
    }
  }
}

void VlanAdapter::OnEmergencyFrameRead(const VlanEmergencyMessage& emmsg) {
  DCHECK(InsideStrand());

  llog() << "\n*** From peer - EMERGENCY. We do it too. EmInfo: " << emmsg.GetInfo().Textualize() << "\n\n";

  // Remember this is the reaction to remote peer's emergency, not local emergency.
  DCHECK(!remote_emergency_);
  DCHECK(!remote_emergency_info_);
  remote_emergency_ = true;
  remote_emergency_info_ = make_unique<EmergencyInfo>(emmsg.GetInfo());
  RaiseEmergency(EI(EmergencyReason::remote_emergency));
  //StopUnsafe();
}

// Dispatch remote connect. Loop our accept table. If accept entry exists,
// acquire new handle.
void VlanAdapter::OnConnectFrameRead(const VlanConnectMessage& connmsg) {
  DCHECK(InsideStrand());

  if (peer_kconfig_ == nullptr) {
    // First time, expecting kconfig
    if (connmsg.HasAttachedKConfig()) {
      // Ok
      peer_kconfig_ = make_unique<VlanKernelConfig>(connmsg.AttachedKConfig());
      llog() << "1st time kconfig in Connect frame " << peer_kconfig_->Textualize() << "\n";

      ConfigurePeer();
    }
    else {
      // Expected, NOT PRESENT
      RaiseEmergency(EI(EmergencyReason::protocol_violated,
                     ViolationTag_KConfigExpectedConnect));
      return;
    }
  }
  else {
    if (!connmsg.HasAttachedKConfig()) {
      // OK, Not expected, not present
    }
    else {
      // Not expected, PRESENT
      RaiseEmergency(EI(EmergencyReason::protocol_violated,
                     ViolationTag_KConfigNotExpectedConnect));
      return;
    }

  }
  // Check if the requested endpoint is listening
  auto ep = connmsg.GetEndpoint();
  auto& it_bound = current_accepts_.find(connmsg.GetEndpoint());
  if (it_bound == current_accepts_.end()) {
    // Respond connection refused. Anyway, send our kconfig_.
    PushFrame<VlanConnectResultMessage>(false,
                                        make_unique<VlanKernelConfig>(kconfig_));
    return;
  }
  // Create a new handle
  if (!accepted_handles_.CreateHandle(*it_bound->second.new_handle,
      make_shared<HandleContext>(kconfig_)))
  {
    // Peer should've known we are out of handles.
    RaiseEmergency(EI(EmergencyReason::protocol_violated,
                   ViolationTag_HandleLimitReached));
    return;
  }
  // First erase from |current_accepts_|. Copy uhandler first.
  auto hcopy(it_bound->second.uhandler);
  current_accepts_.erase(it_bound);
  //
  // Call user's accept handler directly (it can rebind to same address)
  //
  hcopy(VlanError::NoError());

  // Handles synchronized with client table, send our kconfig_.
  PushFrame<VlanConnectResultMessage>(true,
                                      make_unique<VlanKernelConfig>(kconfig_));
}

void VlanAdapter::OnConnectResultFrameRead(const VlanConnectResultMessage& crmsg) {
  if (!connect_queue_.size()) {
    // #VIOLATION TAG unexpected_connect_result_frame 4001
    RaiseEmergency(EI(EmergencyReason::protocol_violated,
                   ViolationTag_UnexpectedConnResultFrame));
    return;
  }
  // Copy, remove from queue
  AcceptConnectData dcopy(connect_queue_.front());
  connect_queue_.pop_front();

  // First time see opponent's kconfig? Save it.
  if (peer_kconfig_ == nullptr) {
    // Expecting peer's kconfig
    if (crmsg.HasAttachedKConfig()) {
      // Ok
      peer_kconfig_ = make_unique<VlanKernelConfig>(crmsg.AttachedKConfig());
      llog() << "1st time kconfig in ConnectResult frame " << peer_kconfig_->Textualize() << "\n";

      ConfigurePeer();
    }
    else {
      // Expected kconfig, but not attached to msg
      RaiseEmergency(EI(EmergencyReason::protocol_violated,
                     ViolationTag_KConfigExpectedConnectResult));
      return;
    }
  }
  else {
    // Not expecting peer's kconfig
    if (!crmsg.HasAttachedKConfig()) {
      // Ok
    }
    else {
      // Unexpected, but attached
      RaiseEmergency(EI(EmergencyReason::protocol_violated,
                     ViolationTag_KConfigNotExpectedConnectResult));
      return;
    }
  }
  DCHECK(connected_handles_);
  // Create handle (synchronized with server table)
  if (!connected_handles_->CreateHandle(*dcopy.new_handle,
      make_shared<HandleContext>(kconfig_)))
  {
    // Peer should have known our limit
    RaiseEmergency(EI(EmergencyReason::protocol_violated,
                   ViolationTag_HandleLimitReached));
    return;
  }
  // Complete user's async connect call
  VlanError vlerr;
  if (!crmsg.GetSuccess()) {
    vlerr = VlanError(VlanErrc::connection_refused);
  }
  dcopy.uhandler(vlerr);
}

void VlanAdapter::ConfigurePeer() {
  // First time we see peer's kconfig. Initialize or xxx properly.
  DCHECK(peer_kconfig_);
  DCHECK(!connected_handles_);
  connected_handles_ = make_unique<ConnectedHandlesTable>(peer_kconfig_->accept_max);
}

void VlanAdapter::RaiseEmergency(const EI& eminfo) {
  DCHECK(InsideStrand());
  if (raising_emergency_) {
    llog() << "already reporting!\n";
    return;
  }
  raising_emergency_ = true;

  // Create context and pass it to user's fn (if present)
  VlanEmergencyContext emerctx(eminfo);
  if (emergency_fn_) {
    // For remote peer's emergency, notification is ignored
    emergency_fn_(emerctx);
  }
  bool switchoff_here = true;
  // Notify peer except when we're here because of peer's emergency
  if (!remote_emergency_) {
    // User may have told us to disable peer notification
    // By default its enabled
    if (emerctx.GetEnablePeerNotify()) {

      // We will switch off after sending emergency frame to counterparty
      switchoff_here = false;
      llog() << "Pushing final VlanEmergencyMessage frame\n";

      PushWrite(CreateFrame<VlanEmergencyMessage>(eminfo),
                [&, eminfo](const VlanError&) {
                  // written. call user's error handler
                  DCHECK(InsideStrand());
                  raising_emergency_ = false;

                  // Finally, switch off. Emergencies are unrecoverable.
                  SwitchOffUnsafe();
                });
    }
  }
  if (switchoff_here) {
    SwitchOffUnsafe();
  }
}

void VlanAdapter::InitWrite() {
  DCHECK(!writing_);
  DCHECK(write_queue_.size() != 0);
  WriteOp& topop = write_queue_.front();

  auto code = topop.frame->GetProtoMessage().GetCode();
  llog() <<
    "InitWrite (frame " << code <<
    " " << VlanMessageTitleFromCode(code) << ", cur " << write_queue_.size() <<
    " in q)\n";

  auto on_write = wrap_post(*strand_.get(),
                            co::bind(&VlanAdapter::HandleWriteFrame,
                            this, _1));
  writing_ = true;
  frame_writer_->AsyncWriteFrame(*topop.frame.get(), on_write);
}

void VlanAdapter::PushWrite(Shptr<Frame> frame, HandlerWithVlErr opt_cbk /*= nullptr*/)
{
  DCHECK(InsideStrand());
  using co::async::wrap_post;

  write_queue_.push_back(WriteOp{ frame, opt_cbk });
  if (write_queue_.size() == 1) {
    // If size was 0, initiate write again
    InitWrite();
  }
}

void VlanAdapter::HandleWriteFrame(Errcode err) {
  DCHECK(InsideStrand());
  DCHECK(writing_);
  writing_ = false;
  WriteOp op(write_queue_.front());
  write_queue_.pop_front();

  // Whatever happen, call opt_cbk first
  if (op.opt_cbk) {
    auto code = op.frame->GetProtoMessage().GetCode();
    llog() << "frame written (" << code << ", " << VlanMessageTitleFromCode(code) << ") [opt cbk present]..\n";
    if (err) {
      op.opt_cbk(VlanError(VlanErrc::transport_error, VlanErrorInfo(err)));
    }
    else {
      op.opt_cbk(VlanError::NoError());
    }
  }
  else {
    llog() << "frame written (cod " << op.frame->GetProtoMessage().GetCode() << ")\n";
  }
  // If error, raise emergency
  if (err) {
    llog() << "EMERGENCY * HandleWriteFrame ERROR " << err << "\n";
    RaiseEmergency(EI(EmergencyReason::write_frame_failed, InvalidViolationTag,
                   err.value()));
    return;
  }
  // Ok, this item is done. If queue is not empty, reinitialize.
  if (!write_queue_.empty() && !writing_) {
    llog() << "Reinitializing Write Queue\n";

    // We may be switching (-ed) off now
    if (!switching_off_) {
      InitWrite();
    }
  }
  else {
    llog() << "No need to reinitialize Write\n";
  }
}

void VlanAdapter::PostSwitchingOffError(HandlerWithVlErr h, io_context& ioc_cbk) {
  boost::asio::post(ioc_cbk, [h]() {
    h(VlanError(VlanErrc::switching_off));
                    });
}

bool VlanAdapter::DealWithSwitchedOff(HandlerWithVlErr h, io_context& ioc_cbk) {
  if (switching_off_) { // atomic
    PostSwitchingOffError(h, ioc_cbk);
    return true;
  }
  return false;
}

// ---

bool VlanAdapter::InsideStrand() const {
  return strand_->running_in_this_thread();
}

void VlanAdapter::SwitchOffUnsafe() {
  // Any number of calls. After first call, ensure there's no new objects added since
  // the first call.
  if (switched_off_) {
    // Second call or any next.
    DCHECK(switching_off_);
    DCHECK(!writing_);
    DCHECK(emergency_fn_ == nullptr);
    DCHECK(!current_accepts_.size());
    DCHECK(!connect_queue_.size());
    DCHECK(!accepted_handles_.GetHandleCount());
    DCHECK(connected_handles_ == nullptr);
  }
  SetEmergencyFn(nullptr);
  current_accepts_.clear();
  connect_queue_.clear();
  accepted_handles_.CloseAllHandles();
  if (connected_handles_) {
    connected_handles_->CloseAllHandles();
    connected_handles_ = nullptr;
  }
  writing_ = false;
  switched_off_ = true;
}

template <typename Protm, typename ...Args>
Shptr<Frame> VlanAdapter::PushFrameWithCbk(HandlerWithVlErr opt_cbk, Args...args) {
  DCHECK(InsideStrand());
  Shptr<Frame> frame = CreateFrame<Protm>(std::forward<Args>(args)...);
  PushWrite(frame, opt_cbk);
  return frame;
}

template <typename Protm, typename ...Args>
Shptr<Frame> VlanAdapter::PushFrame(Args...args) {
  return
    PushFrameWithCbk<Protm, Args...>(
      [](const VlanError&) {
      },
      std::forward<Args>(args)...);
}

template <typename Protm, typename ...Args>
static Shptr<Frame> VlanAdapter::CreateFrame(Args... args) {
  return make_shared<Frame>(make_unique<Protm>(std::forward<Args>(args)...));
}

