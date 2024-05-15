#include "proto/proto_message_writer.h"

#include "co/xlog/xlog.h"

using namespace std;

DEFINE_XLOGGER_SINK("protmsgwrit", gProtoMessageWriterSink);
#define XLOG_CURRENT_SINK gProtoMessageWriterSink

ProtoMessageWriter::ProtoMessageWriter(ChunkWriter& chunk_writer)
  : chunk_writer_(chunk_writer), _dbg_min_sev_(_MAXFATAL)
{
}

void ProtoMessageWriter::AsyncWriteMessage(const ProtoMessage& msg,
                                           HandlerWithErrcode handler) {
  Shptr<string> sered = make_shared<string>(); //|sered| constructed here

  serializer_.SerializeMessage(msg, *sered.get()/*append_to*/);

  chunk_writer_.AsyncWriteChunk(&(*sered.get())[0],
    sered->size(), [sered, handler](Errcode err) {
      handler(err);
    });
}
