#include "co/async/wrap_post.h"
#include "co/async/stream_chunk_writer_queue_st.h"
#include "co/base/hex_dump.h"
#include "co/base/ref_tracker.h"
#include "co/xlog/xlog.h"


#define llogt() Log(_TRACE) << "StmChunkWrQST `" << GET_DEBUG_TAG(_DbgTag()) << "` "
#define llogd() Log(_DBG) << "StmChunkWrQST `" << GET_DEBUG_TAG(_DbgTag()) << "` "

using namespace std;

namespace co {
namespace async {

DEFINE_XLOGGER_SINK("stmchunkwriterqst", gCoAsyncStreamChunkWriterQstLogSink);
#define XLOG_CURRENT_SINK gCoAsyncStreamChunkWriterQstLogSink

StreamChunkWriterQueueST::~StreamChunkWriterQueueST() {
  // Help detect uaf
  DCHECK(!_dbg_dtored_);
  _dbg_dtored_ = true;
}

StreamChunkWriterQueueST::StreamChunkWriterQueueST(
  StreamChunkWriter& underlying_writer)
:
  underlying_writer_(underlying_writer),
  writing_(false)
{
}

void StreamChunkWriterQueueST::AsyncWriteChunk(const void* chunk,
                                               size_t chunk_len,
                                               HandlerWithErrcode handler)
{
  async_write_chunk_internal(chunk, chunk_len, handler);
}


void StreamChunkWriterQueueST::async_write_chunk_internal(const void* d,
                                                          size_t s,
                                                          HandlerWithErrcode h)
{
  write_queue_.push_back(write_op(d, s, h));

  if (write_queue_.size() == 1) { // was empty
    llogt() << "Chunk [" << s << " bytes] added to write queue, initiating write\n";

    init_write_chunk();
  }
  else {
    llogt() << "Chunk [" << s << " bytes] added to write queue, write already initiated\n";
  }
}

void StreamChunkWriterQueueST::DoShutdown() {
  async_write_chunk_internal(nullptr, 0, HandlerWithErrcode());
  //underlying_writer_.DoShutdown();
}

Stream &StreamChunkWriterQueueST::GetStream() {
  return underlying_writer_.GetStream();
}


void StreamChunkWriterQueueST::init_write_chunk()
{
  bool eof_second = false;
  while (write_queue_.front().is_eof()) {
    if (eof_second) {
      syslog(_WARN) << "maybe multieof bug\n";
    }
    // Consume all EOFs, what else to do?
    write_queue_.pop_front();
    llogd() << "EOF is met, doing underlying.shutdown()\n";
    underlying_writer_.DoShutdown();
    if (write_queue_.empty()) {
      llogd() << "OK, Empty queue after EOF is met, OK, done\n";
      return;
    }
    eof_second = true;
  }
  writing_ = true;
  const void* chunk = write_queue_.front().chunk;
  size_t chunk_size = write_queue_.front().chunk_size;
  llogt() << "Passing chunk [" << chunk_size << " bytes] to underlying chunk writer\n";
  underlying_writer_.AsyncWriteChunk(
              chunk,
              chunk_size,
              co::async::wrap_post(
                GetUnderlyingStrand(),
                co::bind(&StreamChunkWriterQueueST::handle_write_chunk,
                         this,
                         _1)));
}

void StreamChunkWriterQueueST::handle_write_chunk(Errcode err) {
  DCHECK(!_dbg_dtored_);
  DCHECK(IsInsideUnderlyingStrand());
  DCHECK(writing_);

  llogt() << "handle_write err=" << err << "\n";

  writing_ = false;

  // Still can be |err|, if so, pass it to handler
  DCHECK(write_queue_.size());
  write_op op(write_queue_.front());
  write_queue_.pop_front();
  op.user_handler(err);

  // Init write again only if no error.
  if (!err) {
    // |writing_| may be just set from |op.user_handler|, check it
    if (!write_queue_.empty() && !writing_) {
      // Init write again.
      init_write_chunk();
    }
  }
}

Strand& StreamChunkWriterQueueST::GetUnderlyingStrand() {
  return *underlying_writer_.GetStrand().get();
}

bool StreamChunkWriterQueueST::IsInsideUnderlyingStrand() {
  return GetUnderlyingStrand().running_in_this_thread();
}

// ------------------------------------

StreamChunkWriterQueueST::write_op::write_op(const void* d,
                                             size_t s,
                                             HandlerWithErrcode h)
  : chunk(d), chunk_size(s), user_handler(h)
{
}

}}

