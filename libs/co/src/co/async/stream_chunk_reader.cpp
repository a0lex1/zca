#include "co/async/stream_chunk_reader.h"
#include "co/async/wrap_post.h"
#include "co/base/hex_dump.h"

#include "co/xlog/xlog.h"

#define llogt() Log(_TRACE) << "StmChunkRdr `" << GET_DEBUG_TAG(_DbgTag()) << "` "

namespace co {
namespace async {

DEFINE_XLOGGER_SINK("stmchunkreader", gCoAsyncStreamChunkReaderLogSink);
#define XLOG_CURRENT_SINK gCoAsyncStreamChunkReaderLogSink

StreamChunkReader::~StreamChunkReader() {
  DCHECK(!_dbg_dtored_);
  _dbg_dtored_ = true;
}

StreamChunkReader::StreamChunkReader(Shptr<Strand> strand, Stream &stm, uint32_t max_body_size)
  :
  strand_(strand), stm_(stm), max_body_size_(max_body_size)
{
  DCHECK(!_dbg_dtored_ || (int)_dbg_dtored_ == (int)true);
  DCHECK(!_dbg_dtored_);
}

StreamChunkReader::Stream &StreamChunkReader::GetStream() { return stm_; }

void StreamChunkReader::AsyncReadChunk(std::string &chunk, StreamChunkReaderHandler handler) {
  DCHECK(!reading_msg_);
  DCHECK(!user_handler_);
  reading_msg_ = true;
  if (got_eof_) {
    // Notify about the EOF we've got on previous read.
    got_eof_ = false;
    //handler(boost::asio::error::eof); // why was?
    CompleteUserRequest(StreamChunkReaderError(StreamChunkReaderErrc::stream_error,
                                               StreamChunkReaderErrorInfo(boost::asio::error::eof)));
    return;
  }
  user_handler_ = handler;
  user_chunk_ = &chunk;

  ReadBodySize();
}

void StreamChunkReader::ReadBodySize() {
  // Not wrapped in strand because AsyncRead(Write)All() calls handler in |strand_|
  co::async::AsyncReadAll(strand_,
                          stm_, boost::asio::buffer(&cur_read_body_size_, sizeof(uint32_t)),
                          co::bind(&StreamChunkReader::HandleReadBodySize,
                                   this, _1, _2));
}

void StreamChunkReader::HandleReadBodySize(Errcode err, size_t num_bytes) {
  DCHECK(InsideStrand());
  DCHECK(num_bytes == 0 || num_bytes == sizeof(uint32_t));
  if (err) {
    if (err == boost::asio::error::eof && num_bytes != 0) {
      // Unexpected EOF in read-body-size stage
      CompleteUserRequest(StreamChunkReaderError(
                            StreamChunkReaderErrc::unexpected_eof_header));
    }
    else {
      // Other error
      CompleteUserRequest(StreamChunkReaderError(StreamChunkReaderErrc::stream_error,
                                                 StreamChunkReaderErrorInfo(err)));
    }
  }
  else {
    if (cur_read_body_size_ > max_body_size_) {
      // Body size limit violation
      CompleteUserRequest(StreamChunkReaderError(
                            StreamChunkReaderErrc::body_size_limit_violated,
                            StreamChunkReaderErrorInfo(cur_read_body_size_)));
    }
    else {
      //  success
      llogt() << "Body size will be " << cur_read_body_size_ << ", reading body...\n";
      ReadBody();
    }
  }
}

void StreamChunkReader::ReadBody() {
  DCHECK(InsideStrand());
  user_chunk_->resize(cur_read_body_size_);
  char* cur_ptr = const_cast<char*>(user_chunk_->c_str());
  // No strand because AsyncReadAll calls handler from inside strand
  co::async::AsyncReadAll(strand_,
                          stm_,
                          boost::asio::buffer(cur_ptr, cur_read_body_size_),
                          co::bind(&StreamChunkReader::HandleReadBody,
                                   this, _1, _2));
}

void StreamChunkReader::HandleReadBody(Errcode err, size_t num_bytes) {
  DCHECK(InsideStrand());

  StreamChunkReaderError scr_err;

  if (err == boost::asio::error::eof) {
    if (num_bytes != cur_read_body_size_) {
      // Unexpected EOF in read in read-body stage
      scr_err = StreamChunkReaderError(StreamChunkReaderErrc::unexpected_eof_body);
    }
    else {
      // Expected EOF. We can't notify the user that the message has been read and the EOF has been reached in a one call to its handler.
      // Remember that the EOF is reached to complete the following call to async_read_message() with the EOF code.
      got_eof_ = true;
      scr_err = StreamChunkReaderError::NoError();
    }
  }
  else {
    // Not eof, maybe it's success?
    if (!err) {
      llogt() << "Body has been read===>\n" << HexDump().DoHexDump(nullptr, *user_chunk_) << "\n";
      scr_err = StreamChunkReaderError::NoError();
    }
    else {
      // No, this is some other error
      scr_err = StreamChunkReaderError(StreamChunkReaderErrc::stream_error,
                                       StreamChunkReaderErrorInfo(err));
    }
  }
  user_chunk_ = nullptr;
  CompleteUserRequest(scr_err);
}

void StreamChunkReader::CompleteUserRequest(const StreamChunkReaderError &scr_err) {
  // maybe not in strand if called from Start()
  DCHECK(user_handler_);
  auto handler_copy = user_handler_;
  user_handler_ = nullptr;
  reading_msg_ = false;
  handler_copy(scr_err);
}



}}

