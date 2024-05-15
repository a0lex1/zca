#pragma once

#include "proto/proto_message_factory.h"
#include "proto/proto_message_unserializer.h"
#include "proto/proto_error.h"

#include "co/async/stream_chunk_reader.h"
#include "co/async/read_write_all.h"

#include <atomic>

class ProtoMessageReader {
public:
  using StreamChunkReader = co::async::StreamChunkReader;
  using StreamChunkReaderError = co::async::StreamChunkReaderError;

  typedef Func<void(ProtoError)> HandlerFunc;

  ProtoMessageReader(StreamChunkReader& chunk_reader,
                     ProtoMessageFactory& msg_fac)
    :
    chunk_reader_(chunk_reader), msg_fac_(msg_fac), unserializer_(msg_fac)
  {
  }

  // Single fiber read only

  void AsyncReadMessage(Uptr<ProtoMessage>& msg, HandlerFunc handler) {
    user_read_msg_ = &msg;
    user_handler_ = handler;
    chunk_reader_.AsyncReadChunk(read_chunk_,
                                 co::bind(&ProtoMessageReader::HandleReadChunk, this, _1));
  }

  // Needed by: GetChunkReader().GetStream().Close()
  StreamChunkReader& GetChunkReader() { return chunk_reader_; }

private:
  void HandleReadChunk(const StreamChunkReaderError& scr_err) {
    if (scr_err) {
      CompleteUserRequest(ProtoError(ProtoErrc::stream_chunk_reader_error,
                          ProtoErrorInfo(scr_err)));
      return;
    }
    /* OK. Don't apply the following rule. Allow 0-length messages. */
    /*if (read_chunk_.size() < sizeof(ProtoMessageCode) + 1) { // code + at least 1 byte of data
      CompleteUserRequest(ProtoError(ProtoErrc::bad_chunk));
      return;
    }*/
    Uptr<ProtoMessage> unsered_msg = unserializer_.UnserializeMessage(read_chunk_, nullptr/*allowed_messages*/);
    if (unsered_msg) {
      *user_read_msg_ = std::move(unsered_msg);
      CompleteUserRequest(ProtoError::NoError());
    }
    else {
      CompleteUserRequest(ProtoError(ProtoErrc::unserialize_failed));
    }
  }
  void CompleteUserRequest(ProtoError pt_err) {
    DCHECK(user_handler_);
    auto handler_copy = user_handler_;
    user_handler_ = nullptr;
    user_read_msg_ = nullptr;
    handler_copy(pt_err);
  }
private:
  StreamChunkReader& chunk_reader_;
  ProtoMessageFactory& msg_fac_;
  ProtoMessageUnserializer unserializer_;
  HandlerFunc user_handler_;
  Uptr<ProtoMessage>* user_read_msg_;
  std::string read_chunk_;
};


