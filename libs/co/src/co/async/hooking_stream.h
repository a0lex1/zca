#pragma once

#include "co/async/stream.h"

namespace co {
namespace async {

// A stream that has an underlying stream
class HookingStream: public Stream {
public:
  virtual ~HookingStream() = default;

  HookingStream(Uptr<Stream> underlying_stm)
    :
    Stream(underlying_stm->GetIoContext()),
    underlying_stm_uptr_(std::move(underlying_stm)),
    underlying_stm_ptr_(underlying_stm_uptr_.get())
  {
  }

  HookingStream(Stream& underlying_stm)
    :
    Stream(underlying_stm.GetIoContext()),
    underlying_stm_ptr_(&underlying_stm)
  {
  }

  Stream& GetUnderlyingStream() { return *underlying_stm_ptr_; }
  const Stream& GetUnderlyingStream() const { return *underlying_stm_ptr_; }

private:
  Uptr<Stream> underlying_stm_uptr_;
  Stream* underlying_stm_ptr_;
};

}}

