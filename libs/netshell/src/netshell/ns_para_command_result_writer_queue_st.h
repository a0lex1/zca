#pragma once

#include "netshell/netshell_factory.h"

#include <deque>

namespace netshell {

class NsParaCommandResultWriterQueueST : public NsParaCommandResultWriter {
public:
  virtual ~NsParaCommandResultWriterQueueST() = default;

  NsParaCommandResultWriterQueueST(NsParaCommandResultWriter& underlying_writer);

  // Copies the |cmd_result|, so you can free it after calling AsyncWriteParallelResult
  // In constrast to StreamChunkWriterQueueST, which does NOT copy the buffer
  void AsyncWriteParallelResult(uint64_t cmd_index,
    const NsCmdResult& cmd_result, HandlerWithErrcodeSize handler) override;

  Shptr<Strand> GetStrand() override { return GetUnderlyingStrand(); }

  void CleanupAbortedStop() override;

  Shptr<Strand> GetUnderlyingStrand() {
    return underlying_writer_.GetStrand();
  }

  bool IsInsideUnderlyingStrand() {
    return GetUnderlyingStrand()->running_in_this_thread();
  }

private:
  void InitWrite();
  void HandleWrite(Errcode, size_t);
  void DropAllHandlersWithError(Errcode, size_t);

private:
  struct WriteOp {
    uint64_t cmd_index;
    NsCmdResult cmd_result;
    HandlerWithErrcodeSize user_handler;

    WriteOp(uint64_t _cmd_index, const NsCmdResult& _cmd_result, HandlerWithErrcodeSize _user_handler);
    WriteOp() {}
  };
  using WriteQueue = std::deque<WriteOp>;

private:
  NsParaCommandResultWriter& underlying_writer_;
  WriteQueue write_queue_;
  bool writing_{ false };
};

}
