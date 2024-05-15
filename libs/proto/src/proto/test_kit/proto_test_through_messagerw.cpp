#include "proto/proto_message_factory.h"
#include "proto/proto_message_reader.h"
#include "proto/proto_message_writer.h"

#include "proto/test_kit/connected_streams.h"

#include "co/async/stream_chunk_writer_queue_st.h"
#include "co/async/tcp_stream_connector.h"
#include "co/async/tcp_stream_acceptor.h"
#include "co/async/startable_stopable.h"

#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;

static const uint32_t kMaxBodySize = 1024 * 10;

class ProtoThroughMessageRwTester : public Startable {
public:
  ProtoThroughMessageRwTester(io_context& ioc, ProtoMessageFactory& fac)
    :
    pair_(make_unique<TcpStream>(ioc), make_unique<TcpStream>(ioc),
          make_shared<TcpStreamConnector>(), make_unique<TcpStreamAcceptor>(ioc),
          co::net::TcpEndpoint("127.0.0.1", 0)),
    stm1_chunk_reader_(make_shared<Strand>(ioc), pair_.GetStream1(), kMaxBodySize),
    stm1_reader_(stm1_chunk_reader_, fac),
    stm2_chunk_writer_(make_shared<Strand>(ioc), pair_.GetStream2()),
    stm2_chunk_q_st_writer_(stm2_chunk_writer_),
    stm2_writer_(stm2_chunk_q_st_writer_),
    fac_(fac)
  {
    read_counter_ = 0;
    write_counter_ = 0;
    got_eof_ = false;
  }

  virtual void Start(RefTracker rt) override {
    fac_.CreateTestMessages(msgs_);
    pair_.Setup(co::bind(&ProtoThroughMessageRwTester::HandlePairSetup, this, _1, rt));
  }

  bool GotEof() const { return got_eof_; }

private:
  // Write stream2 then read stream1
  void HandlePairSetup(Errcode err, RefTracker rt) {
    DCHECK(!err);
    WriteAll(rt);
  }
  void WriteAll(RefTracker rt) {
    for (size_t i=0; i<msgs_.size(); i++) {
      stm2_writer_.AsyncWriteMessage(
        *msgs_[i].get(), co::bind(&ProtoThroughMessageRwTester::HandleWriteMessage, this, _1, i, rt));
    }
  }
  void HandleWriteMessage(Errcode err, size_t op_num, RefTracker rt) {
    syslog(_DBG) << "[" << op_num << "]: err = " << err  << "\n";
    DCHECK(!err);
    if (++write_counter_ == msgs_.size()) {
      // Shutdown writer (send eof)
      Errcode ignored_err;
//      pair_.GetStream2().Shutdown(ignored_err);
      stm2_chunk_q_st_writer_.DoShutdown(); // rename to WriteEof
      // Start reading when everything is written.
      ReadNextMessage(rt);
    }
  }
  void ReadNextMessage(RefTracker rt) {
    stm1_reader_.AsyncReadMessage(cur_read_msg_,
                                  co::bind(&ProtoThroughMessageRwTester::HandleReadMessage, this, _1, rt));
  }
  void HandleReadMessage(ProtoError pt_err, RefTracker rt) {
    syslog(_DBG) << "  pt_err = " << pt_err << " (" << pt_err.MakeErrorMessage() << ")\n";
    if (pt_err.IsStreamEOF()) {
      syslog(_INFO) << "IsStreamEOF()=true. Done.\n";
      got_eof_ = true;
      return;
    }
    DCHECK(!pt_err);
    DCHECK(cur_read_msg_->Compare(*msgs_[read_counter_].get()));
    ++read_counter_;
    if (read_counter_ == msgs_.size() + 1) {
      return;
    }
    ReadNextMessage(rt);
  }

private:
  ConnectedStreams pair_;
  StreamChunkReader stm1_chunk_reader_;
  StreamChunkWriter stm2_chunk_writer_;
  StreamChunkWriterQueueST stm2_chunk_q_st_writer_;
  ProtoMessageFactory& fac_;
  ProtoMessageReader stm1_reader_;
  ProtoMessageWriter stm2_writer_;
  vector<Uptr<ProtoMessage>> msgs_;
  Uptr<ProtoMessage> cur_read_msg_;
  Errcode err_;
  size_t read_counter_;
  size_t write_counter_;
  bool got_eof_;
};

// ------------------------------------------------------------------

void ProtoTestThroughMessageRw(ProtoMessageFactory& fac) {
  io_context ioc;
  RefTrackerContext ctx(CUR_LOC());
  ProtoThroughMessageRwTester tester(ioc, fac);
  tester.Start(RefTracker(CUR_LOC(), ctx.GetHandle(), []() {}));
  ioc.run();
  DCHECK(tester.GotEof());
}




