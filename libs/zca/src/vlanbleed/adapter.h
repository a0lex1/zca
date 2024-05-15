#pragma once

#include "./handle_context.h"
#include "./emergency.h"
#include "./error.h"
#include "./handle_table.h"
#include "./native_api.h"
#include "./frame.h"

#include "co/async/service.h"
#include "co/async/wrap_post.h"

#include "co/base/debug_tag_owner.h"

#include <deque>

// This is called inside strand
using ViolationHookFn = Func<void(const VlanError& vlerr)>;

struct VlanAdapterParams {
  vlhandle_t max_accept_handles{ 0 }; // we can limit peer's connect with this value
  vlhandle_t max_connect_handles{ 0 }; // peer can limit this value
  uint16_t queue_size{ 0 };
  uint32_t read_buffer_size{ 0 };
};

static void GetDefaultVlanAdapterParams(VlanAdapterParams& pa) {
  static const VlanAdapterParams def_params = {
    //65535, 65535, 100, 60000 // real probably
    10, 10, 25, 128
  };
  pa = def_params;
}

class VlanAdapter
  :
  private FrameHandler,
  private VlanNativeApi
{
  VlanAdapter() = delete;
  VlanAdapter(const VlanAdapter&) = delete;
public:
  virtual ~VlanAdapter();

  using Stream = co::async::Stream;

  VlanAdapter(Shptr<Strand> strand,
              const VlanAdapterParams& adap_params);

  // Default action is to notify peer about error
  void SetEmergencyFn(VlanEmergencyFn emergency_fn) { emergency_fn_ = emergency_fn; }

  FrameHandler& GetFrameHandler() { return *this; }
  void SetFrameWriter(FrameWriterST& w) { frame_writer_ = &w; }

  void SwitchOffThreadsafe();

  // main multiplexer thread safe API
  VlanNativeApi& GetNativeApi() { return *this; }

  co::DebugTagOwner& DebugTag() { return _dbg_tag_; }

private:
  // [VlanNativeApi impl]
  void VnReserveAddressTSafe(const VlanEndpoint& ep, VlanError& vlerr) override;
  void VnReleaseAddressTSafe(const VlanEndpoint& ep) override;
  void VnAccept(vlhandle_t& new_handle, const VlanEndpoint& ep, HandlerWithVlErr handler, io_context& ioc_cbk) override;
  void VnConnect(vlhandle_t& new_handle, const VlanEndpoint& ep, HandlerWithVlErr handler, io_context& ioc_cbk) override;
  void VnRead(vlhandle_t handle, mutable_buffers_1 buf, HandlerWithVlErrSize handler, io_context& ioc_cbk) override;
  void VnWrite(vlhandle_t handle, const_buffers_1 buf, HandlerWithVlErrSize handler) override;
  void VnCancelAccept(vlhandle_t handle, VlanError& vlerr) override;
  void VnShutdownSend(vlhandle_t handle, VlanError& vlerr) override;
  void VnClose(vlhandle_t handle) override;

  // Inside-fiber workers
  void VnConnectUnsafe(vlhandle_t&, VlanEndpoint, HandlerWithVlErr, io_context&);
  void VnAcceptUnsafe(vlhandle_t&, VlanEndpoint, HandlerWithVlErr, io_context&);
  void VnReadUnsafe(vlhandle_t, mutable_buffers_1, HandlerWithVlErrSize, io_context&);
  void VnWriteUnsafe(vlhandle_t, const_buffers_1, HandlerWithVlErrSize);
  void VnShutdownSendUnsafe();

  // [FrameHandler impl]
  void InputFrame(const Frame& frame) override;

  //
  void ConfigurePeer();
  void PushWrite(Shptr<Frame>, HandlerWithVlErr opt_cbk = nullptr);
  void InitWrite();
  void HandleWriteFrame(Errcode);
  void HandleWriteDataMessage(const VlanError&, Shptr<HandleContext>, HandlerWithVlErrSize, size_t);
  void OnConnectFrameRead(const VlanConnectMessage&);
  void OnConnectResultFrameRead(const VlanConnectResultMessage&);
  void OnDataFrameRead(const VlanDataMessage&);
  void OnEmergencyFrameRead(const VlanEmergencyMessage&);
  void RaiseEmergency(const EmergencyInfo&);
  void PostSwitchingOffError(HandlerWithVlErr, io_context&);
  bool DealWithSwitchedOff(HandlerWithVlErr, io_context&);
  void SwitchOffUnsafe();
  Shptr<HandleContext> FindHandleContext(vlhandle_t);
  void TransferBufferData(mutable_buffers_1, const_buffers_1, size_t&);

  template <typename Protm, typename ...Args> Shptr<Frame> PushFrame(Args...args);
  template <typename Protm, typename ...Args> Shptr<Frame> PushFrameWithCbk(HandlerWithVlErr opt_cbk, Args...args);
  template <typename Protm, typename ...Args> static Shptr<Frame> CreateFrame(Args... args);
  bool InsideStrand() const;

  using mutable_buffers_1 = boost::asio::mutable_buffers_1;

  // Map entry for both accept and connect stages
  struct AcceptConnectData {
    vlhandle_t* new_handle{ nullptr };
    HandlerWithVlErr uhandler;
    io_context* ioc_cbk{ nullptr };
  };

  // Accept map
  using AcceptMap = std::map<VlanEndpoint, AcceptConnectData>;

  // Connect queue
  using ConnectQueue = std::deque<AcceptConnectData>;

  // Accepted & connected stream handles tables. Their format is the same.
  using HandleTable = HandleTable<vlhandle_t, Shptr<HandleContext>>;
  using ConnectedHandlesTable = HandleTable;
  using AcceptedHandlesTable = HandleTable;

  // Write queue
  struct WriteOp {
    Shptr<Frame> frame;
    HandlerWithVlErr opt_cbk;
  };
  using WriteQueue = std::deque<WriteOp>;

private:
  Shptr<Strand> strand_;
  VlanAdapterParams adap_params_;
  VlanEmergencyFn emergency_fn_;
  FrameWriterST* frame_writer_{ nullptr };

  const VlanKernelConfig kconfig_;
  Uptr<VlanKernelConfig> peer_kconfig_;
  AcceptMap current_accepts_;
  ConnectQueue connect_queue_;

  AcceptedHandlesTable accepted_handles_;
  Uptr<ConnectedHandlesTable> connected_handles_;

  bool kconfig_sent_{ false }; // peer has our kconfig
  bool raising_emergency_{ false };
  bool remote_emergency_{ false }; // raising due to remote's emergency NOT USED YET
  Uptr<EmergencyInfo> remote_emergency_info_;
  bool writing_{ false };
  std::atomic<bool> switching_off_{ false };
  bool switched_off_{ false };

  WriteQueue write_queue_;

  co::DebugTagOwner _dbg_tag_;

};

