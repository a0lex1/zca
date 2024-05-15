#pragma once

#include "co/async/hooking_stream.h"
#include "co/async/stream_acceptor.h"

namespace co {
namespace async {

// An acceptor that has an underlying acceptor
class HookingStreamAcceptor: public StreamAcceptor {
public:
  virtual ~HookingStreamAcceptor() = default;

  HookingStreamAcceptor(Uptr<StreamAcceptor> underlying_acceptor)
    :
    StreamAcceptor(underlying_acceptor->GetIoContext()),
    underlying_acceptor_uptr_(std::move(underlying_acceptor)),
    underlying_acceptor_ptr_(underlying_acceptor_uptr_.get())
  {
  }

  HookingStreamAcceptor(StreamAcceptor& underlying_acceptor)
    :
    StreamAcceptor(underlying_acceptor.GetIoContext()),
    underlying_acceptor_ptr_(&underlying_acceptor)
  {
  }

  StreamAcceptor& GetUnderlyingAcceptor() { return *underlying_acceptor_ptr_; }
  const StreamAcceptor& GetUnderlyingAcceptor() const { return *underlying_acceptor_ptr_; }


  void AsyncAccept(Stream& stm, HandlerWithErrcode handler) override {
    HookingStream& hooking_stm = static_cast<HookingStream&>(stm);
    underlying_acceptor_ptr_->AsyncAccept(hooking_stm.GetUnderlyingStream(), handler);
  }

  bool IsOpen() override { return underlying_acceptor_ptr_->IsOpen(); }
  void Open(Errcode& err) override { underlying_acceptor_ptr_->Open(err); }
  void Bind(Endpoint addr, Errcode& err) override { underlying_acceptor_ptr_->Bind(addr, err); }
  void StartListening(Errcode& err) override { underlying_acceptor_ptr_->StartListening(err); }
  void GetLocalAddress(Endpoint& addr, Errcode& err) override { underlying_acceptor_ptr_->GetLocalAddress(addr, err); }

  void GetLocalAddressToConnect(Endpoint& addr, Errcode& err) override { underlying_acceptor_ptr_->GetLocalAddressToConnect(addr, err);  }

  void Close() override { underlying_acceptor_ptr_->Close(); }
  void CancelAccept(Errcode& err) override { underlying_acceptor_ptr_->CancelAccept(); }

private:
  Uptr<StreamAcceptor> underlying_acceptor_uptr_;
  StreamAcceptor* underlying_acceptor_ptr_;
};


}}

