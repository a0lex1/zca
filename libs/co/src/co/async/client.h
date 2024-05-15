#pragma once

#include "co/async/threadsafe_stopable_impl.h"
#include "co/async/loop_object.h"
#include "co/async/session.h"
#include "co/async/stream_connector.h"
#include "co/async/startable_stopable.h"

#include "co/xlog/define_logger_sink.h"

#include <atomic>

namespace co {
namespace async {

DECLARE_XLOGGER_SINK("client", gCoAsyncClientLogSink);

class Client : public LoopObject, public Stopable {
public:
  virtual ~Client();

  using Endpoint = net::Endpoint;

  // One strand used for both tss_impl_ and client_session_
  Client(Endpoint addr,
         Shptr<StreamConnector> connector,
         Shptr<Session> client_session);

  // [Loopobject:: impl]
  void PrepareToStart(Errcode&) override;
  void CleanupAbortedStop() override;
  bool IsResetSupported() const override;
  void ResetToNextRun() override;
  void Start(RefTracker rt) override;
  void StopThreadsafe() override;

  void SetRemoteAddress(const Endpoint& remaddr);
  const Endpoint& GetRemoteAddress() const { return addr_; }
  bool IsAddressSet() const;
  Errcode GetConnectError() const { return conn_err_; }

  Shptr<Session> GetSession() { return client_session_; }

  template <typename T> // sugar (const/nonconst)
  const Shptr<T> GetSessionAs() const {
    return std::static_pointer_cast<T>(client_session_);
  }

  template <typename T> // sugar (const/nonconst)
  Shptr<T> GetSessionAs() {
    return (static_cast<const Client*>(this)->GetSessionAs<T>());
  }

private:
  void StopUnsafe() override;


  void HandleConnect(Errcode err, RefTracker rt) ;

  Strand& GetSessionStrand() {
    DCHECK(client_session_ != nullptr);
    DCHECK(client_session_->GetFiberStrandShptr() != nullptr);
    DCHECK(client_session_->GetFiberStrandShptr() == tss_impl_.GetStrandShptr());
    return client_session_->GetFiberStrand();
  }
  bool InsideSessionStrand() {
    return GetSessionStrand().running_in_this_thread();
  }

private:
  struct {
    bool prepared_to_start_ : 1;
    bool addr_set_ : 1;
    bool started_ : 1;
    bool stop_ : 1;
  } bools_ { false, false, false, false };

  ThreadsafeStopableImpl tss_impl_;
  Endpoint addr_;
  Shptr<Session> client_session_;
  Shptr<StreamConnector> connector_;
  Errcode conn_err_;

  std::atomic<size_t> _dbg_handleconnect_called_{ 0 };
  std::atomic<size_t> _dbg_preparetostart_called_{ 0 };
  std::atomic<size_t> _dbg_start_called_{ 0 };
  std::atomic<size_t> _dbg_resettonextrun_called_{ 0 };
  std::atomic<size_t> _dbg_stopthreadsafe_called_{ 0 };
};


}}

