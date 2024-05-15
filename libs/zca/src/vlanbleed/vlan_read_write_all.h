#pragma once

#include "./error.h"
#include "./native_api.h"

#include <boost/asio/buffer.hpp>
#include <boost/enable_shared_from_this.hpp>

enum class VlanReadWriteOpType {
  kRead,
  kWrite
};

class VlReadWriteAllOp : public co::enable_shared_from_this<VlReadWriteAllOp> {
public:
  virtual ~VlReadWriteAllOp() = default;

  VlReadWriteAllOp(
    VlanReadWriteOpType _optype,
    VlanNativeApi& _vlapi,
    vlhandle_t _vlhandle,
    boost::asio::mutable_buffers_1 _buf,
    HandlerWithVlErrSize _handler,
    io_context& _ioc_cbk)
    :
    optype_(_optype), vlapi_(_vlapi), vlhandle_(_vlhandle), buf_(_buf), handler_(_handler),
    ioc_cbk_(_ioc_cbk), original_size_(_buf.size())
  {
  }
  void Start() {
    DoAgain();
  }
private:
  // Writes don't need ioc_cbk, writing on transport's ioc
  void DoAgain() {
    if (optype_ == VlanReadWriteOpType::kRead) {
      vlapi_.VnRead(vlhandle_, buf_, co::bind(&VlReadWriteAllOp::HandleResult,
                    shared_from_this(), _1, _2),
                    ioc_cbk_);
    }
    else {
      if (optype_ == VlanReadWriteOpType::kWrite) {
        boost::asio::const_buffers_1 cbufs(buf_);
        vlapi_.VnWrite(vlhandle_, cbufs, co::bind(&VlReadWriteAllOp::HandleResult,
                       shared_from_this(), _1, _2));
      }
      else {
        NOTREACHED();
      }
    }
  }
  /* 
  void HandleResult(Errcode err, size_t num_bytes, bool read_not_write) {





    // SHIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIT, what if !err && num_bytes==0 ? like in linux

    // SHIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIT, what if !err && num_bytes==0 ? like in linux

    // SHIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIT, what if !err && num_bytes==0 ? like in linux


    // can call directly, we can't be called directly from AsyncReadSome/AsyncWriteSome =>
    // because asio docs say: Regardless of whether the asynchronous operation completes immediately or not, the handler will not be invoked from within this function. Invocation of the handler will be performed in a manner equivalent to using boost::asio::io_service::post().
    if (err) {
      // make copy and clear because... it's safer
      auto handler_copy(user_handler_);
      user_handler_ = nullptr;
      handler_copy(err, 0);
      return;
    }

    DCHECK(boost::asio::buffer_size(user_buf_) >= num_bytes);
    user_buf_ += num_bytes;

    if (0 == boost::asio::buffer_size(user_buf_)) {
      // The entire buffer is filled. Transfer done.
      auto handler_copy(user_handler_);
      user_handler_ = nullptr;
      handler_copy(NoError(), orig_buf_size_);
      return;
    }

    // More bytes required
    DoAgain();
  }*/
  void HandleResult(const VlanError& vlerr, size_t num_bytes) {
    if (vlerr) {
      auto handler_copy(handler_);
      handler_ = nullptr;
      handler_copy(VlanError::NoError(), 0);
      return;
    }
    DCHECK(boost::asio::buffer_size(buf_) >= num_bytes);
    buf_ += num_bytes;

    if (0 == boost::asio::buffer_size(buf_)) {
      // The entire buffer is filled. Transfer done.
      auto handler_copy(handler_);
      handler_ = nullptr;
      handler_copy(VlanError::NoError(), original_size_);
      return;
    }

    DoAgain();
  }

private:
  VlanReadWriteOpType optype_;
  VlanNativeApi& vlapi_;
  vlhandle_t vlhandle_;
  boost::asio::mutable_buffers_1 buf_;
  HandlerWithVlErrSize handler_;
  io_context& ioc_cbk_;
  size_t original_size_;
};

static void VlanReadAll(VlanNativeApi& vlapi,
                        vlhandle_t vlhandle,
                        boost::asio::mutable_buffers_1 buf,
                        HandlerWithVlErrSize handler,
                        io_context& ioc_cbk) {
  Shptr<VlReadWriteAllOp>  op = make_shared<VlReadWriteAllOp>(VlanReadWriteOpType::kRead,
                                                              vlapi,
                                                              vlhandle,
                                                              buf,
                                                              handler,
                                                              ioc_cbk);
  op->Start();
}

static void VlanWriteAll(VlanNativeApi& vlapi,
                         vlhandle_t vlhandle,
                         boost::asio::const_buffers_1 buf,
                         HandlerWithVlErrSize handler,
                         io_context& ioc_cbk) {
  Shptr<VlReadWriteAllOp> op = make_shared<VlReadWriteAllOp>(VlanReadWriteOpType::kWrite,
                                                             vlapi,
                                                             vlhandle,
                                                             boost::asio::mutable_buffers_1(const_cast<void*>(buf.data()),
                                                             buf.size()),
                                                             handler,
                                                             ioc_cbk);
  op->Start();
}

