#pragma once

#include "./messages.h"
#include "./error.h"
#include "./frame.h"

#include "proto/proto_message_reader.h"
#include "proto/proto_message_writer.h"

#include "co/async/threadsafe_stopable_impl.h"
#include "co/async/wrap_post.h"
#include "co/async/startable_stopable.h"

#include "co/base/debug_tag_owner.h"

class VlanTransportBase {
public:
  virtual ~VlanTransportBase() = default;

  using RefTracker = co::RefTracker;

  virtual FrameWriterST& GetFrameWriter() = 0;
  virtual void SetFrameHandler(FrameHandler&) = 0;
};

class VlanTransport
  :
  public VlanTransportBase,
  public co::async::Startable,
  public co::async::Stopable,
  public co::async::ThreadsafeStopable
{
public:
  virtual ~VlanTransport() = default;

  using ThreadsafeStopableImpl = co::async::ThreadsafeStopableImpl;

  VlanTransport(Strand& strand)
    : tss_impl_(*this, strand)
  {
  }

protected:
  ThreadsafeStopableImpl& GetTSSImpl() { return tss_impl_; }
  Strand& GetStrand() { return tss_impl_.GetStrand(); }

private:
  ThreadsafeStopableImpl tss_impl_;
};

/* ------------------------------------------------ */

class VlanProtoTransport
  :
  public VlanTransport,
  private FrameWriterST
{
public:
  virtual~VlanProtoTransport() = default;

  VlanProtoTransport(ProtoMessageReader& rdr, ProtoMessageWriter& wr, 
                     Strand& strand);

  void SetFrameHandler(FrameHandler& frame_handler) override { frame_handler_ = &frame_handler; }
  FrameWriterST& GetFrameWriter() override { return *this; }

  void Start(RefTracker rt) override;
  void StopThreadsafe() override;

  const VlanError& GetLastError() const { return last_vlan_error_; }

  // [FrameWriterST impl]
  // |frame| is supposed to be exist until |handler|. Caller's responsibility.
  void AsyncWriteFrame(const Frame& frame, HandlerWithErrcode handler) override;

  co::DebugTagOwner& DebugTag() { return _dbg_tag_; }

private:
  void StopUnsafe() override;
  void ReadNextMessage(RefTracker rt);
  void HandleReadProtoMessage(ProtoError proto_err, RefTracker rt);
  void HandleWriteMessage(Errcode err);
  bool InsideStrand();

  using ThreadsafeStopableImpl = co::async::ThreadsafeStopableImpl;

private:
  ProtoMessageReader& prot_rdr_;
  ProtoMessageWriter& prot_wr_;
  FrameHandler* frame_handler_{ nullptr };
  HandlerWithErrcode uwrite_handler_;
  Uptr<ProtoMessage> cur_proto_msg_;
  VlanError last_vlan_error_;

  co::DebugTagOwner _dbg_tag_;
};

/* ------------------------------------------------ */

// class VlanStreamTransport aggregates class VlanProtoTransport
class VlanStreamTransport
  :
  public VlanTransport,
  private FrameWriterST
{
public:
  virtual ~VlanStreamTransport() = default;

  using RefTracker = co::RefTracker;
  using Stream = co::async::Stream;
  using StreamChunkReader = co::async::StreamChunkReader;
  using StreamChunkWriter = co::async::StreamChunkWriter;

  // Configure VlanProtoTransport and use it
  VlanStreamTransport(Stream& stm,
                      Shptr<Strand> strand,
                      uint32_t max_chunk_body_size)
    :
    VlanTransport(*strand.get()),
    strand_(strand), // keepalive
    chunk_rdr_(stm, max_chunk_body_size), prot_rdr_(chunk_rdr_, prot_msgfac_),
    chunk_wr_(stm), prot_wr_(chunk_wr_),
    prot_trans_(prot_rdr_, prot_wr_, *strand.get())
  {
  }

  FrameWriterST& GetFrameWriter() override { return *this; }
  void SetFrameHandler(FrameHandler& h) override { prot_trans_.SetFrameHandler(h); }

  // [VlanTransport::Startable impl]
  void Start(RefTracker rt) override;
  void StopThreadsafe() override;

  co::DebugTagOwner& DebugTag() { return _dbg_tag_; }

private:
  // [VlanTransport::Stopable impl]
  void StopUnsafe() override;

  // [FrameWriter impl]
  void AsyncWriteFrame(const Frame& frame, HandlerWithErrcode handler) override {
    prot_trans_.AsyncWriteFrame(frame, handler);
  }

private:
  Shptr<Strand> strand_;
  VlanProtoMessageFactory prot_msgfac_;
  StreamChunkReader chunk_rdr_;
  StreamChunkWriter chunk_wr_;
  ProtoMessageReader prot_rdr_;
  ProtoMessageWriter prot_wr_;
  VlanProtoTransport prot_trans_;

  co::DebugTagOwner _dbg_tag_;
};


