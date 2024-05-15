#pragma once

#include "netshell/netshell_factory.h"
#include "co/base/async_coro.h"

namespace netshell {
namespace sync {

class SyncNsParaCommandResultWriter: public NsParaCommandResultWriterBase, public co::AsyncCoroAdaptor<NsParaCommandResultWriter> {
public:
  virtual ~SyncNsParaCommandResultWriter() = default;

  using AsyncCoro = co::AsyncCoro;
  using NetshellError = netshell::NetshellError;
  using NsCmdResult = netshell::NsCmdResult;

  using AsyncCoroAdaptor::AsyncCoroAdaptor;

  Errcode WriteParallelResult(uint64_t cmd_index, const NsCmdResult& cmd_result,
    size_t* bytes_written);

  // Cleanupable::CleanupAbortedStop from NsParaCommandResultReaderBase
  void CleanupAbortedStop() override;

private:

};

}}
