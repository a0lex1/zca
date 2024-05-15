#pragma once

#include "co/async/stream_chunk_error.h"
#include "co/async/read_write_all.h"
#include "co/base/strings.h"
#include "co/xlog/define_logger_sink.h"

#include <atomic>

namespace co {
namespace async {

DECLARE_XLOGGER_SINK("stmchunkreader", gCoAsyncStreamChunkReaderLogSink);

using StreamChunkReaderHandler = Func<void(const StreamChunkReaderError& scr_error)>;

class StreamChunkReaderBase {
public:
  virtual ~StreamChunkReaderBase() = default;

  virtual void AsyncReadChunk(std::string& chunk, StreamChunkReaderHandler handler) = 0;
};

class StreamChunkReader : public StreamChunkReaderBase {
public:
  using Stream = co::async::Stream;

  virtual ~StreamChunkReader();
  
  StreamChunkReader(Shptr<Strand> strand, Stream& stm, uint32_t max_body_size);

  Stream& GetStream();
  
  // Single fiber read only
  void AsyncReadChunk(std::string& chunk, StreamChunkReaderHandler handler) override;

  co::DebugTagOwner& _DbgTag() { return _dbg_tag_; }

private:
  void ReadBodySize();
  void HandleReadBodySize(Errcode err, size_t num_bytes);
  void ReadBody();
  void HandleReadBody(Errcode err, size_t num_bytes);

  void CompleteUserRequest(const StreamChunkReaderError& scr_err);

  bool InsideStrand() { return strand_->running_in_this_thread(); }

private:
  Shptr<Strand> strand_;
  Stream& stm_;
  uint32_t max_body_size_{ 0 };
  std::atomic<bool> reading_msg_{ false };
  bool got_eof_{ false };
  std::string* user_chunk_{ nullptr };
  StreamChunkReaderHandler user_handler_;
  uint32_t cur_read_body_size_{ 0 };

  bool _dbg_dtored_{false};
  co::DebugTagOwner _dbg_tag_;
};


}}



