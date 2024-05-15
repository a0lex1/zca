#pragma once

#include "co/async/stream_connector.h"
#include "co/async/stream_acceptor.h"

class ConnectedStreams {
public:
  using Endpoint = co::net::Endpoint;
  using Stream = co::async::Stream;
  using StreamConnector = co::async::StreamConnector;
  using StreamAcceptor = co::async::StreamAcceptor;
  using RefTracker = co::RefTracker;

  ConnectedStreams(Uptr<Stream> stm1,
                   Uptr<Stream> stm2, 
                   Shptr<StreamConnector> stm_conn,
                   Uptr<StreamAcceptor> stm_acpt,
                   Endpoint loopback_addr)
    :
    stm1_(std::move(stm1)),
    stm2_(std::move(stm2)),
    stm_conn_(stm_conn),
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
    stm_acpt_->AsyncAccept(*stm2_.get(), co::bind(&ConnectedStreams::HandleAccept, this, _1));
    stm_conn_->AsyncConnect(locaddr, *stm1_.get(), co::bind(&ConnectedStreams::HandleConnect, this, _1));
  }
  Stream& GetStream1() { return *stm1_.get(); }
  Stream& GetStream2() { return *stm2_.get(); }
private:
  void HandleAccept(Errcode err) {
    if (err) {
      stm1_->Close(); // Cancel connect
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
  Uptr<Stream> stm1_;
  Uptr<Stream> stm2_;
  Shptr<StreamConnector> stm_conn_;
  Uptr<StreamAcceptor> stm_acpt_;
  HandlerWithErrcode user_handler_;
  Endpoint loopback_addr_;
  size_t counter_;
};



