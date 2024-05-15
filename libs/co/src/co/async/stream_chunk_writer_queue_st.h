#pragma once

#include "co/async/stream_chunk_writer.h"
#include "co/xlog/define_logger_sink.h"

#include <deque>

namespace co {
namespace async {

DECLARE_XLOGGER_SINK("stmchunkwriterqst", gCoAsyncStreamChunkWriterQstLogSink);

// Single Threaded Write Queue
// No parallel reads

class StreamChunkWriterQueueST : public ChunkWriter {
public:
  virtual ~StreamChunkWriterQueueST();
  
  StreamChunkWriterQueueST(StreamChunkWriter& underlying_writer);
  
  // Not threadsafe, but supports queueing writes
  void AsyncWriteChunk(const void* chunk,
                       size_t chunk_len,
                       HandlerWithErrcode handler) override;

  // Not threadsafe
  void DoShutdown() override;

  Stream& GetStream() override;


private:
  void init_write_chunk();
  void handle_write_chunk(Errcode);
  void async_write_chunk_internal(const void*, size_t, HandlerWithErrcode);

  Strand& GetUnderlyingStrand();
  bool IsInsideUnderlyingStrand();

private:
  struct write_op {
    const void* chunk{nullptr};
    size_t chunk_size{0};
    HandlerWithErrcode user_handler;

    bool is_eof() const { return chunk == nullptr; }
    write_op(const void*, size_t, HandlerWithErrcode);
    write_op() {}
  };
  using write_queue = std::deque<write_op>;

private:
  StreamChunkWriter& underlying_writer_;
  write_queue write_queue_;
  bool writing_;

  bool _dbg_dtored_{false};
};



}}


