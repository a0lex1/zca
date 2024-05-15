#pragma once

#include "co/async/hooking_stream.h"
#include "co/base/arc4.h"

namespace co {
namespace async {

class Rc4Stream : public HookingStream {
public:
  virtual ~Rc4Stream() = default;

  Rc4Stream(Uptr<Stream> underlying_stm, const void* r_rc4key, size_t r_rc4key_len,
    const void* w_rc4key, size_t w_rc4key_len)
    :
    HookingStream(std::move(underlying_stm)),
    r_rc4_(arc4(r_rc4key, r_rc4key_len)),
    w_rc4_(arc4(w_rc4key, w_rc4key_len))
  {
  }

  Rc4Stream(Stream& underlying_stm, const void* r_rc4key, size_t r_rc4key_len,
    const void* w_rc4key, size_t w_rc4key_len)
    :
    HookingStream(underlying_stm),
    r_rc4_(arc4(r_rc4key, r_rc4key_len)),
    w_rc4_(arc4(w_rc4key, w_rc4key_len))
  {
  }
  
  void AsyncReadSome(boost::asio::mutable_buffers_1 buf, HandlerWithErrcodeSize handler) override;

  void AsyncWriteSome(boost::asio::const_buffers_1 buf, HandlerWithErrcodeSize handler) override;
  
  void Shutdown(Errcode& err) override {
    GetUnderlyingStream().Shutdown(err);
  }

  void Cancel(Errcode& err) override {
    GetUnderlyingStream().Cancel(err);
  }

  void Close() override {
    GetUnderlyingStream().Close();
  }

  bool IsOpen() const override {
    return GetUnderlyingStream().IsOpen();
  }

  void GetLocalAddress(co::net::Endpoint& addr, Errcode& err) override {
    return GetUnderlyingStream().GetLocalAddress(addr, err);
  }
  void GetRemoteAddress(co::net::Endpoint& addr, Errcode& err) override {
    return GetUnderlyingStream().GetRemoteAddress(addr, err);
  }

private:
  arc4 r_rc4_;
  arc4 w_rc4_;
};

}}
