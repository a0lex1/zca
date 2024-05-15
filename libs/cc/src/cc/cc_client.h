#pragma once

#include "cc/cc_client_bot_options.h"
#include "cc/cc_client_session.h"

#include "co/async/client.h"
#include "co/async/loop_object.h"
#include "co/xlog/define_logger_sink.h"

namespace cc {

DECLARE_XLOGGER_SINK("cccli", gCcClientSink);

class CcClient : public co::async::LoopObjectNoreset, private co::async::Stopable {
 public:
  virtual ~CcClient() = default;

  using Endpoint = co::net::Endpoint;
  using StreamConnector = co::async::StreamConnector;
  using Stream = co::async::Stream;
  using ThreadsafeStopableImpl = co::async::ThreadsafeStopableImpl;
  using RefTracker = co::RefTracker;
  using Client = co::async::Client;
  using time_duration = boost::posix_time::time_duration;

  CcClient(Endpoint remaddr,
           Shptr<StreamConnector> connector,
           Uptr<Stream> stm,
           Shptr<Strand> strand,
           CcClientCommandDispatcher& command_dispatcher,
           CcClientEvents* events,
           uint32_t max_chunk_body_size, /* proto messaging */
           CcClientBotOptions bot_opts)
    :
    tss_impl_(*this, strand),
    shared_data_(make_shared<CcClientSharedData>(bot_opts)),
    remaddr_(remaddr),
    connector_(connector),
    stm_(std::move(stm)),
    session_strand_(strand),
    command_dispatcher_(command_dispatcher),
    events_(events),
    max_chunk_body_size_(max_chunk_body_size)
  {
  }

  // Override ping interval for client or let server choose it
  void SetPingInterval(boost::posix_time::time_duration ping_interval);
  void UseServerPingInterval();

  // Traffic encryption keys need to be either set or encryption needs to be disabled
  void SetTrafficEncryptionKeys(const void* r_rc4key, size_t, const void* w_rc4key, size_t);
  void DisableTrafficEncryption();

  void SetOpaqueHandshakeData(Uptr<std::string> opaque_hshake_data);

  CcClientBotOptions& GetBotOptions() { return shared_data_->bot_opts_; }
  
  // Can be called both before and after PrepareToStart()
  void SetRemoteAddress(const Endpoint& remaddr);

  void PrepareToStart(Errcode& err) override;
  
  void Start(RefTracker rt) override;

  void CleanupAbortedStop() override;

  void StopThreadsafe() override;

  // Only after ioended
  const std::vector<CcError>& GetLastErrorStack() const;

  Errcode GetConnectError() const;
  
private:
  void StopUnsafe() override;

private:
  ThreadsafeStopableImpl tss_impl_;
  Shptr<CcClientSharedData> shared_data_;
  
  Endpoint remaddr_;
  Shptr<StreamConnector> connector_;
  Uptr<Stream> stm_;
  Shptr<Strand> session_strand_;
  CcClientCommandDispatcher& command_dispatcher_;
  CcClientEvents* events_{ nullptr };
  uint32_t max_chunk_body_size_;

  bool traffic_encryption_enabled_{ true };
  Uptr<std::string> r_rc4key_buffer_;
  Uptr<std::string> w_rc4key_buffer_;

  Uptr<std::string> opaque_hshake_data_;

  Uptr<Client> cli_;

  struct {
    bool prepared_to_start : 1;
  } bools_{ false };
};

}


