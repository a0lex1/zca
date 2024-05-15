#include "netshell/sync/sync_command_writer.h"

namespace netshell {
namespace sync {

NetshellError SyncNsCommandWriter::WriteCommand(const std::string& cmd) {
  auto cbk(GetCoro().CreateContinuationCallback<NetshellError>());
  GetCoro().DoYield([&]() {
    GetAdaptedObject().AsyncWriteCommand(cmd, cbk.GetFunction());
    });
  return std::get<0>(cbk.ResultTuple());
}

void SyncNsCommandWriter::CleanupAbortedStop() {
  GetAdaptedObject().CleanupAbortedStop();
}


}}

