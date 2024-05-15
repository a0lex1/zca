#pragma once

#include "netshell/netshell_factory.h"
#include "co/base/async_coro.h"

namespace netshell {
namespace sync {

class SyncNsCommandWriter: public NsCommandWriterBase, public co::AsyncCoroAdaptor<NsCommandWriter> {
public:
  virtual ~SyncNsCommandWriter() = default;

  using AsyncCoro = co::AsyncCoro;
  using NetshellError = netshell::NetshellError;

  using AsyncCoroAdaptor::AsyncCoroAdaptor;

  NetshellError WriteCommand(const std::string& cmd);

  // Cleanupable::CleanupAbortedStop from NsCommandResultWriterBase
  void CleanupAbortedStop() override;

private:

};

}}

