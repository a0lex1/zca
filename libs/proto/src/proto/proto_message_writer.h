#pragma once

#include "proto/proto_message_serializer.h"

#include "co/async/stream_chunk_writer.h"

#include "co/xlog/define_logger_sink.h"
#include "co/base/debug_tag_owner.h"

DECLARE_XLOGGER_SINK("protmsgwrit", gProtoMessageWriterSink);

class ProtoMessageWriter {
public:
  using Stream = co::async::Stream;
  using ChunkWriter = co::async::ChunkWriter;

  ProtoMessageWriter(ChunkWriter& chunk_writer);
 
  // 1. |handler| doesn't need to be ProtoError because no erros can occur during serialization
  // 2. You won a lottery. You can free |msg| after AsyncWriteMessage returns. It does make a copy of this.
  void AsyncWriteMessage(const ProtoMessage& msg, HandlerWithErrcode handler);

  ChunkWriter& GetChunkWriter() { return chunk_writer_; }


  // Don't look here. Debug details! namespace detail!
  //void _DbgEnableLoggingWithSeverity(int min_sev) { _dbg_min_sev_ = min_sev; }
  co::DebugTagOwner& _DbgTag() { return _dbg_tag_; }
  
private:
  ChunkWriter& chunk_writer_;
  ProtoMessageSerializer serializer_;

  int _dbg_min_sev_;
  co::DebugTagOwner _dbg_tag_;
};

