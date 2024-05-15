#include "netshell/ns_para_command_result_writer_queue_st.h"

#include "co/async/wrap_post.h"

using namespace co::async;

namespace netshell {

NsParaCommandResultWriterQueueST::NsParaCommandResultWriterQueueST(NsParaCommandResultWriter& underlying_writer)
  :
  underlying_writer_(underlying_writer)
{
}

void NsParaCommandResultWriterQueueST::AsyncWriteParallelResult(uint64_t cmd_index,
  const NsCmdResult& cmd_result, HandlerWithErrcodeSize handler)
{
  write_queue_.push_back(WriteOp(cmd_index, cmd_result, handler));

  if (write_queue_.size() == 1) { // was empty
    InitWrite();
  }
}

void NsParaCommandResultWriterQueueST::CleanupAbortedStop() {
  underlying_writer_.CleanupAbortedStop();
}

void NsParaCommandResultWriterQueueST::InitWrite() {
  writing_ = true;
  underlying_writer_.AsyncWriteParallelResult(write_queue_.front().cmd_index,
    write_queue_.front().cmd_result,
    wrap_post(*GetUnderlyingStrand(),
      co::bind(&NsParaCommandResultWriterQueueST::HandleWrite,
        this, _1, _2)));
}

void NsParaCommandResultWriterQueueST::HandleWrite(Errcode err, size_t num_bytes) {
  writing_ = false;
  
  DCHECK(write_queue_.size());
  WriteOp op(write_queue_.front());
  write_queue_.pop_front();
  op.user_handler(err, num_bytes);

  if (err) {
    DropAllHandlersWithError(err, num_bytes);
    return;
  }
  else {
    if (!write_queue_.empty() && !writing_) {
      InitWrite();
    }
  }
}

void NsParaCommandResultWriterQueueST::DropAllHandlersWithError(Errcode err, size_t num_bytes) {
  while (write_queue_.size()) {
    WriteOp op(write_queue_.front());
    write_queue_.pop_front();
    op.user_handler(err, num_bytes);
  }
}

// ---

NsParaCommandResultWriterQueueST::WriteOp::WriteOp(
  uint64_t _cmd_index,
  const NsCmdResult& _cmd_result,
  HandlerWithErrcodeSize _user_handler)
  :
  cmd_index(_cmd_index), cmd_result(_cmd_result), user_handler(_user_handler)
{
}

}
