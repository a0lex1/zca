#pragma once

#include "netshell/netshell_factory.h"
#include "co/base/async_coro.h"

namespace netshell {
namespace sync {

class SyncNsParaCommandResultReader: public NsParaCommandResultReaderBase, public co::AsyncCoroAdaptor<NsParaCommandResultReader> {
public:
  virtual ~SyncNsParaCommandResultReader() = default;

  using AsyncCoro = co::AsyncCoro;
  using NetshellError = netshell::NetshellError;
  using NsCmdResult = netshell::NsCmdResult;

  using AsyncCoroAdaptor::AsyncCoroAdaptor;

  NetshellError ReadParallelResult(uint64_t& cmd_index, NsCmdResult& cmd_result);

  // Cleanupable::CleanupAbortedStop from NsParaCommandResultReaderBase
  void CleanupAbortedStop() override;

private:

};

}}
