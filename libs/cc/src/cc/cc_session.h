#pragma once

#include "cc/cc_error.h"
#include "cc/cc_proto/message_factory.h"
#include "cc/cc_proto/handshake_message.h"

#include "proto/proto_message_reader.h"
#include "proto/proto_message_writer.h"

#include "co/async/stream_chunk_writer_queue_st.h"
#include "co/async/session.h"
#include "co/base/debug_tag_owner.h"
#include "co/xlog/define_logger_sink.h"

namespace cc {

DECLARE_XLOGGER_SINK("ccsess", gCcSessionSink);


/* 
* class CcSession
* 
* Shared by CcClientSession and CcBotSession (CcBotSession is server side session for client)
*
*/
class CcSession :
  public co::async::Session,
  public co::enable_shared_from_this<CcSession>
{
public:
  virtual ~CcSession();
  
  using Stream = co::async::Stream;
  using RefTracker = co::RefTracker;
  using StreamChunkWriterQueueST = co::async::StreamChunkWriterQueueST;

  CcSession(Uptr<Stream> new_stm,
            Shptr<Strand> strand,
            uint32_t max_chunk_body_size);

  // Client and Server sessions store err stack differently
  virtual std::vector<CcError>& GetLastErrorStack() = 0;

  co::DebugTagOwner& _DbgTagOfChunkWriterQST() { return chunk_writer_qst_._DbgTag(); }
  co::DebugTagOwner& _DbgTagOfChunkReader() { return chunk_reader_._DbgTag(); }

protected:

  template <typename T = CcSession>
  Shptr<T> GetSharedFromThisAs() { return std::static_pointer_cast<T>(shared_from_this()); }

  ProtoMessageReader& GetMessageReader() { return msg_reader_; }
  ProtoMessageWriter& GetSequencedMessageWriter() { return msg_writer_; }
  void StartReadMessageLoop(RefTracker); // and start this whenever you want (after using msg reader/writer)
  io_context& GetIoContext();

  void PushLastError(const CcError& ccerr);

  // [Session override]
  void StopUnsafe() override;

  bool IsStopping() const { return stopping_;}

private:
  //BeginIo()

  // [To implement]
  virtual void DispatchProtoMessage(const ProtoMessage& msg, HandlerWithCcErr handler) = 0;

  using StreamChunkWriter = co::async::StreamChunkWriter;
  using StreamChunkReader = co::async::StreamChunkReader;
  //using Session::GetStream; // disallow
    
  void ReadNextMessage(RefTracker);
  void HandleReadMessage(ProtoError, RefTracker);
  void HandleDispatchMessage(const CcError&, RefTracker);

private:
  StreamChunkWriter chunk_writer_;
  StreamChunkWriterQueueST chunk_writer_qst_;
  StreamChunkReader chunk_reader_;
  ProtoMessageWriter msg_writer_;
  ProtoMessageReader msg_reader_;

  cc_proto::MessageFactory cc_msg_fac_;
  Uptr<ProtoMessage> cur_read_msg_;

  bool stopping_{ false };
};


}


