#pragma once

#include "netshell/netshell_factory.h"
#include "co/base/async_coro.h"

namespace netshell {
namespace sync {

class SyncNsCommandResultReader: public NsCommandResultReaderBase, public co::AsyncCoroAdaptor<NsCommandResultReader> {
public:
  virtual ~SyncNsCommandResultReader() = default;

  using AsyncCoro = co::AsyncCoro;
  using NetshellError = netshell::NetshellError;
  using NsCmdResult = netshell::NsCmdResult;

  using AsyncCoroAdaptor::AsyncCoroAdaptor;

  NetshellError ReadResult(NsCmdResult& cmd_result);

  // Cleanupable::CleanupAbortedStop from NsCommandResultReaderBase
  void CleanupAbortedStop() override;

private:

};

}}

