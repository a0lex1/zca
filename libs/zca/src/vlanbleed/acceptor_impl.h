#pragma once

#include "./stream_impl.h"
#include "./native_api.h"

#include "co/async/stream_acceptor.h"

class StreamAcceptorImpl : public co::async::StreamAcceptor
{
public:
  virtual ~StreamAcceptorImpl() {
    if (IsBound()) {
      native_api_.VnReleaseAddressTSafe(bound_to_);
      // DCHECK(ensure_released);
      bound_to_ = VlanEndpoint();
    }
  }

  StreamAcceptorImpl(io_context& ioc, VlanNativeApi& native_api)
    : StreamAcceptor(ioc), native_api_(native_api)
  {
  }

  using Stream = co::async::Stream;
  using Endpoint = co::net::Endpoint;

  bool IsOpen() override { return opened_; }
  void Open(Errcode&) override {
    DCHECK(!opened_);
    DCHECK(!start_listeninged_);
    DCHECK(!IsBound());
    opened_ = true;
  }
  void Bind(Endpoint ep, Errcode& err) override {
    DCHECK(opened_);
    DCHECK(!start_listeninged_);
    DCHECK(!IsBound());
    const auto& vlep(static_cast<VlanEndpoint>(ep));

    VlanError vlerr;
    native_api_.VnReserveAddressTSafe(vlep, vlerr);
    if (vlerr) {
      err = vlerr.ToErrcodeDefault();
    }
    else {
      // OK, bound. Remember the address we've been bound to
      bound_to_ = ep;
    }
  }
  void StartListening(Errcode& err) override {
    DCHECK(opened_);
    DCHECK(IsBound());
    DCHECK(!start_listeninged_);
    start_listeninged_ = true;
    err = co::NoError();
  }
  void GetLocalAddress(Endpoint&, Errcode& err) override {
    err = co::NoError();
  }
  void GetLocalAddressToConnect(Endpoint&, Errcode& err) override {
    err = co::NoError();
  }
  void Close() override {

  }
  void CancelAccept(Errcode& err) override {
    DCHECK(opened_);
    DCHECK(IsBound());
    DCHECK(start_listeninged_);
    DCHECK(handle_ != 0);
    VlanError vlerr;
    native_api_.VnCancelAccept(handle_, vlerr);
    err = vlerr.ToErrcodeDefault();
  }
  void AsyncAccept(Stream& stm, HandlerWithErrcode uhandler) override {
    DCHECK(opened_);
    DCHECK(IsBound());
    DCHECK(start_listeninged_);
    DCHECK(!accepting_);
    accepting_ = true;
    native_api_.VnAccept(handle_, bound_to_,
                         [&, uhandler](const VlanError& vle) {
                           accepting_ = false;
                           if (!vle) {
                             auto& stmimpl = static_cast<StreamImpl&>(stm);
                             stmimpl.SetNativeApi(native_api_);
                           }
                           Errcode e = vle.ToErrcodeDefault();
                           uhandler(e);
                         },
                         GetIoContext());
  }

private:
  bool IsBound() const { return bound_to_ != VlanEndpoint::NullAddress(); }

private:
  VlanNativeApi& native_api_;
  bool opened_{ false };
  bool start_listeninged_{ false };
  bool accepting_{ false };
  VlanEndpoint bound_to_;
  vlhandle_t handle_{ 0 };
};



