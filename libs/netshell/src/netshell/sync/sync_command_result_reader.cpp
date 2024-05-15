#include "netshell/sync/sync_command_result_reader.h"

namespace netshell {
namespace sync {

NetshellError SyncNsCommandResultReader::ReadResult(NsCmdResult& cmd_result) {
  auto cbk(GetCoro().CreateContinuationCallback<NetshellError>());
  GetCoro().DoYield([&]() {
    GetAdaptedObject().AsyncReadResult(cmd_result, cbk.GetFunction());
    });
  return std::get<0>(cbk.ResultTuple());
}

void SyncNsCommandResultReader::CleanupAbortedStop() {
  GetAdaptedObject().CleanupAbortedStop();
}

}}

