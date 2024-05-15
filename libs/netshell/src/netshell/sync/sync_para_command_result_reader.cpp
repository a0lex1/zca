#include "netshell/sync/sync_para_command_result_reader.h"

namespace netshell {
namespace sync {

NetshellError SyncNsParaCommandResultReader::ReadParallelResult(uint64_t& cmd_index, NsCmdResult& cmd_result) {
  auto cbk(GetCoro().CreateContinuationCallback<NetshellError>());
  GetCoro().DoYield([&]() {
    GetAdaptedObject().AsyncReadParallelResult(cmd_index, cmd_result, cbk.GetFunction());
    });
  return std::get<0>(cbk.ResultTuple());
}

void SyncNsParaCommandResultReader::CleanupAbortedStop() {
  GetAdaptedObject().CleanupAbortedStop();
}


}}
