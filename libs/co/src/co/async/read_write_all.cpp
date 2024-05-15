#include "co/async/read_write_all.h"
#include "co/async/wrap_post.h"
#include "co/async/startable_stopable.h"

#include "co/xlog/xlog.h"

#include <boost/enable_shared_from_this.hpp>

static uint64_t _dbg_gCheckpoint0 = 0, _dbg_gCheckpoint1 = 0, _dbg_gCheckpoint2 = 0 ;

namespace co {
namespace async {

enum class ReadWriteOpType {
  kOpRead,
  kOpWrite
};

class ReadWriteAllOp
  :
  public co::enable_shared_from_this<ReadWriteAllOp> {
public:
  //
  // TODO: Add without-strand support (someone may be not event ThreadsafeStopable i.e. doesn't have a strand
  //
  ReadWriteAllOp(Shptr<Strand> strand, // optional
                 ReadWriteOpType optype,
                 StreamIo& stm,
                 boost::asio::mutable_buffers_1 buf,
                 HandlerWithErrcodeSize handler)
    :
    strand_(strand),
    optype_(optype), stm_io_(stm), user_buf_(buf), user_handler_(handler),
    orig_buf_size_(boost::asio::buffer_size(buf))
  {
  }
  
  void Start() {
    DoAgain();
  }
  
private:
  HandlerWithErrcodeSize WrapHandler(HandlerWithErrcodeSize handler) {
    if (strand_ != nullptr) {
      return wrap_post(*strand_.get(), handler);
    }
    else {
      return boost::asio::bind_executor(
            stm_io_.GetIoContext(), handler);
    }
  }
  void DoAgain() {
    // wrap to ioc
    if (optype_ == ReadWriteOpType::kOpRead) {
      stm_io_.AsyncReadSome(
            user_buf_,
            WrapHandler(co::bind(&ReadWriteAllOp::HandleResult, shared_from_this(),
                                 _1, _2, true)));
    }
    else {
      if (optype_ == ReadWriteOpType::kOpWrite) {
        stm_io_.AsyncWriteSome(
              boost::asio::const_buffers_1(user_buf_),
              WrapHandler(co::bind(&ReadWriteAllOp::HandleResult, shared_from_this(),
                                   _1, _2, false)));
      }
      else {
        NOTREACHED();
      }
    }
  }
  
  void HandleResult(Errcode err, size_t num_bytes, bool tag_readnotwrite) {
    if (strand_ != nullptr) {
      DCHECK(strand_->running_in_this_thread());
    }
    //if (!err && num_bytes == 0) { // remake see below
    //  NOTREACHED();
    //}
    (void)tag_readnotwrite;
    // This can be READ EOF WITH bytecount>0.
    // So first append to the buffer. Then deal with errors.
    DCHECK(boost::asio::buffer_size(user_buf_) >= num_bytes);
    user_buf_ += num_bytes;

    _dbg_gCheckpoint0 += 1;

    /*
    * THIS CODE 89.2%  PROBABILITI CONTAINS BAD LOGIC WITH ERROR CHECKING. see in chain with stream_chunk_reader.
    * linux bug here
    */
    if (err || num_bytes==0) {
      // This is an error case for Read/write ALL operation.
      //
      // Can call directly, we can't be called directly from AsyncReadSome/AsyncWriteSome =>
      // because asio docs say: Regardless of whether the asynchronous operation completes immediately or not, the handler will not be invoked from within this function. Invocation of the handler will be performed in a manner equivalent to using boost::asio::io_service::post().
      auto handler_copy(user_handler_);
      user_handler_ = nullptr;

      // (not tested)

      handler_copy(err, 0);

      _dbg_gCheckpoint1 += 1;
      return;
    }

    // Is the entire buffer filled now?
    if (0 == boost::asio::buffer_size(user_buf_)) {
      // The entire buffer is filled. Transfer done.
      auto handler_copy(user_handler_);
      user_handler_ = nullptr;

      handler_copy(NoError(), orig_buf_size_);

      _dbg_gCheckpoint2 += 1;
      return;
    }

    // More bytes required
    DoAgain();
  }
  
private:
  Shptr<Strand> strand_;
  ReadWriteOpType optype_;
  StreamIo& stm_io_;
  boost::asio::mutable_buffers_1 user_buf_;
  HandlerWithErrcodeSize user_handler_;
  size_t orig_buf_size_;
};

// ---------------------------------------------------------------------------------------------------------------------

void AsyncReadAll(Shptr<Strand> strand, StreamIo& stm,
                  boost::asio::mutable_buffers_1 buf, HandlerWithErrcodeSize handler)
{
  DCHECK(strand != nullptr);
  auto op = make_shared<ReadWriteAllOp>(strand, ReadWriteOpType::kOpRead,
                                        std::ref(stm), buf, handler);
  op->Start();
}

void AsyncWriteAll(Shptr<Strand> strand, StreamIo& stm,
                   boost::asio::const_buffers_1 buf, HandlerWithErrcodeSize handler)
{
  DCHECK(strand != nullptr);
  boost::asio::mutable_buffers_1 cb(const_cast<void*>(buf.data()), buf.size());
  auto op = make_shared<ReadWriteAllOp>(strand, ReadWriteOpType::kOpWrite,
                                        std::ref(stm), cb, handler);
  op->Start();
}

// -----------------------------------------------------------------------------------------------------------------------------

void AsyncReadAllNostrand(StreamIo& stm, boost::asio::mutable_buffers_1 buf,
                          HandlerWithErrcodeSize handler) {
  boost::asio::mutable_buffers_1 cb(const_cast<void*>(buf.data()), buf.size());
  auto op = make_shared<ReadWriteAllOp>(nullptr/*strand*/,
                                        ReadWriteOpType::kOpWrite,
                                        std::ref(stm), cb, handler);
  op->Start();
}

void AsyncWriteAllNostrand(StreamIo& stm, boost::asio::const_buffers_1 buf,
                           HandlerWithErrcodeSize handler) {
  boost::asio::mutable_buffers_1 cb(const_cast<void*>(buf.data()), buf.size());
  auto op = make_shared<ReadWriteAllOp>(nullptr/*strand*/,
                                        ReadWriteOpType::kOpWrite,
                                        std::ref(stm), cb, handler);
  op->Start();
}

}}







