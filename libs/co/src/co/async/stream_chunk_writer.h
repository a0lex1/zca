#pragma once

#include "co/async/stream.h"
#include "co/xlog/define_logger_sink.h"

#include <boost/asio/buffer.hpp>

namespace co {
namespace async {

DECLARE_XLOGGER_SINK("stmchunkwriter", gCoAsyncStreamChunkWriterLogSink);

class ChunkWriter {
public:
  virtual ~ChunkWriter() = default;

  // Not threadsafe
  virtual void AsyncWriteChunk(const void* chunk,
                               size_t chunk_len,
                               HandlerWithErrcode handler) = 0;

  // Not threadsafe
  virtual void DoShutdown() = 0;

  virtual Stream& GetStream() = 0;

  co::DebugTagOwner& _DbgTag() { return _dbg_tag_; }

private:
  bool _dbg_logging_enabled_{false};
  co::DebugTagOwner _dbg_tag_;
};

// StreamChunkWriter doesn't support neither parallel reads nor parallel writes.
class StreamChunkWriter : public ChunkWriter {
public:
  virtual ~StreamChunkWriter() = default;

  StreamChunkWriter(Shptr<Strand> strand, Stream& stm)
    : strand_(strand), stm_(stm)
  {

  }

  Shptr<Strand> GetStrand() { return strand_; }
  
  void AsyncWriteChunk(const void* chunk,
                       size_t chunk_len,
                       HandlerWithErrcode handler) override;

  void DoShutdown() override;
  
  Stream& GetStream() override { return stm_; }

private:
  void write_body_size();
  void handle_write_body_size(Errcode, size_t);
  void write_body();
  void handle_write_body(Errcode, size_t);
  void ClearAndCallHandler(Errcode);

private:
  Shptr<Strand> strand_;
  Stream& stm_;
  HandlerWithErrcode user_write_handler_;
  uint32_t cur_write_body_size_{ 0 };
  const void* cur_write_chunk_{ nullptr };
  size_t cur_write_chunk_len_{ 0 };
  bool abort_{false};
};

}}


