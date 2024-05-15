#include "netshell/sync/sync_para_command_result_writer.h"

namespace netshell {
namespace sync {

Errcode SyncNsParaCommandResultWriter::WriteParallelResult(
  uint64_t cmd_index, const NsCmdResult& cmd_result, size_t* bytes_written)
{
  auto cbk(GetCoro().CreateContinuationCallback<Errcode, size_t>());
  GetCoro().DoYield([&]() {
    GetAdaptedObject().AsyncWriteParallelResult(cmd_index, cmd_result, cbk.GetFunction());
    });
  if (bytes_written != nullptr) {
    *bytes_written = std::get<1>(cbk.ResultTuple());
  }
  return std::get<0>(cbk.ResultTuple());
}

void SyncNsParaCommandResultWriter::CleanupAbortedStop() {
  GetAdaptedObject().CleanupAbortedStop();
}


}}
