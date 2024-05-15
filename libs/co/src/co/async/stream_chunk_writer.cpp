#include "co/async/wrap_post.h"
#include "co/async/stream_chunk_writer.h"
#include "co/async/read_write_all.h"
#include "co/base/hex_dump.h"

#include "co/xlog/xlog.h"

//#include <boost/lockfree/queue.hpp>
//#include "simple-stdatomic-for-VS-Clang/stdatomic.h" // in General Include Directories
//#include "lfqueue/lfring_cas1.h"

#define llogt() Log(_TRACE) << "StmChunkWr `" << GET_DEBUG_TAG(_DbgTag()) << "` "

namespace co {
namespace async {

DEFINE_XLOGGER_SINK("stmchunkwriter", gCoAsyncStreamChunkWriterLogSink);
#define XLOG_CURRENT_SINK gCoAsyncStreamChunkWriterLogSink

void StreamChunkWriter::AsyncWriteChunk(const void* chunk,
                                        size_t chunk_len,
                                        HandlerWithErrcode handler) {
  if (user_write_handler_) {
    handler(Errcode(boost::asio::error::already_started));
    return;
  }
  user_write_handler_ = handler;
  cur_write_chunk_ = chunk;
  cur_write_chunk_len_ = chunk_len;
  write_body_size();
}

void StreamChunkWriter::DoShutdown() {
  DCHECK(strand_->running_in_this_thread());
  Errcode err;
  stm_.Shutdown(err);
}

void StreamChunkWriter::write_body_size() {
  // Transmit only first 0xffffffff characters of string, cut others.
  cur_write_body_size_ = static_cast<uint32_t>(cur_write_chunk_len_);

  llogt() "Writing body size (" << cur_write_body_size_ << " bytes)\n";

  co::async::AsyncWriteAll(
    strand_,
    stm_,
    boost::asio::buffer((const void*)&cur_write_body_size_, sizeof(uint32_t)),
    co::async::wrap_post(*strand_.get(),
                         co::bind(&StreamChunkWriter::handle_write_body_size, this, _1, _2)));
}

void StreamChunkWriter::handle_write_body_size(Errcode err, size_t num_bytes) {
  DCHECK(strand_->running_in_this_thread());
  DCHECK(num_bytes == 0 || num_bytes == sizeof(cur_write_body_size_));
  if (err || !num_bytes) {
    ClearAndCallHandler(err);
    return;
  }
  // The momehtnt between write_body_size and write_body where we need to check
  // if AbortWriting was called.
  if (abort_) {
    // Aborted by AbortWriting
    llogt() << "write_body: won't do, aborted by AbortWriting. returning.\n";
    return;
  }
  else {
    llogt() << "Writing body (" << cur_write_chunk_len_ << " bytes)===>\n" << HexDump().DoHexDump(nullptr, cur_write_chunk_, cur_write_chunk_len_) << "\n";
  }
  // Write the body.
  write_body();
}

void StreamChunkWriter::write_body() {
  DCHECK(strand_->running_in_this_thread());
  co::async::AsyncWriteAll(
    strand_,
    stm_,
    boost::asio::buffer(cur_write_chunk_, cur_write_chunk_len_),
    co::async::wrap_post(*strand_.get(),
                         co::bind(&StreamChunkWriter::handle_write_body, this, _1, _2)));
}

void StreamChunkWriter::handle_write_body(Errcode err, size_t num_bytes) {
  DCHECK(strand_->running_in_this_thread());
  DCHECK(num_bytes == 0 || num_bytes == cur_write_chunk_len_);
  llogt() << "handle_write_body err=" << err << ", num_bytes="<<num_bytes<<"\n";
  if (!err && num_bytes != cur_write_body_size_) {
    err = boost::asio::error::message_size;
  }
  ClearAndCallHandler(err);
}

void StreamChunkWriter::ClearAndCallHandler(Errcode err) {
  DCHECK(strand_->running_in_this_thread());
  DCHECK(user_write_handler_);
  auto handler_copy = user_write_handler_;
  user_write_handler_ = HandlerWithErrcode(); // clear
  handler_copy(err);
}

}}




