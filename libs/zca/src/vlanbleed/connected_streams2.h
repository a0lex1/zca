#pragma once

#include "co/async/stream_acceptor.h"
#include "co/async/stream_connector.h"
#include "co/async/tcp.h"

class ConnectedStreams2 {
public:
  using Stream = co::async::Stream;
  using StreamConnector = co::async::StreamConnector;
  using StreamAcceptor = co::async::StreamAcceptor;
  using Endpoint = co::net::Endpoint;
  using RefTracker = co::RefTracker;

  ConnectedStreams2(Stream& stm1,
                   Stream& stm2,
                   Uptr<StreamConnector> stm_conn,
                   Uptr<StreamAcceptor> stm_acpt,
                   Endpoint loopback_addr)
    :
    stm1_(stm1),
    stm2_(stm2),
    stm_conn_(std::move(stm_conn)),
    stm_acpt_(std::move(stm_acpt)),
    loopback_addr_(loopback_addr),
    counter_(0)
  {
  }
  void Setup(HandlerWithErrcode handler) {
    user_handler_ = handler;
    DCHECK(!stm_acpt_->IsOpen());
    stm_acpt_->Open();
    stm_acpt_->Bind(loopback_addr_);
    stm_acpt_->StartListening();
    auto locaddr = stm_acpt_->GetLocalAddressToConnect();
    // -------------------------- MUST BE  ThreadsafeStopable SINGLE FIBER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    stm_acpt_->AsyncAccept(stm2_, co::bind(&ConnectedStreams2::HandleAccept, this, _1));
    stm_conn_->AsyncConnect(locaddr, stm1_, co::bind(&ConnectedStreams2::HandleConnect, this, _1));
  }
  Stream& GetStream1() { return stm1_; }
  Stream& GetStream2() { return stm2_; }
private:
  void HandleAccept(Errcode err) {
    DCHECK(user_handler_);
    if (err) {
      // stm1_ is connecting now
      stm1_.Close();
      ClearAndCallUserHandler(err);
      return;
    }
    stm_acpt_->Close();
    OnOneDone(err);
  }
  void HandleConnect(Errcode err) {
    OnOneDone(err);
  }
  void OnOneDone(Errcode err) {
    DCHECK(user_handler_);
    if (err) {
      ClearAndCallUserHandler(err);
    }
    else {
      if (++counter_ == 2) {
        ClearAndCallUserHandler(err);
      }
    }
  }
  void ClearAndCallUserHandler(Errcode err) {
    auto handler_copy = user_handler_;
    user_handler_ = 0;
    handler_copy(err);
  }
private:
  Stream& stm1_;
  Stream& stm2_;
  Uptr<StreamConnector> stm_conn_;
  Uptr<StreamAcceptor> stm_acpt_;
  HandlerWithErrcode user_handler_;
  Endpoint loopback_addr_;
  size_t counter_;
};
