// TODO: REMOVE THIS CODE, use netshell/sync

#pragma once

#include "netshell/netshell_factory.h"

#include "co/async/create_for_endpoint.h"

#include "co/base/async_coro.h"

// not strongly AsyncCoroAdaptor, exceptional case class
class SyncNetshellClient {
public:
  using AsyncCoro = co::AsyncCoro;
  using NetshellError = netshell::NetshellError;
  using NsCmdResult = netshell::NsCmdResult;

  SyncNetshellClient(const netshell::NsStatusDescriptorTable& status_descriptors,
    co::AsyncCoro& coro, io_context& ioc, size_t max_cmd_len);

  Errcode Disconnect(); // No yield

  NetshellError ExecuteCommand(const std::string& cmd, NsCmdResult& cmd_result);

  NetshellError WriteCommand(const std::string& cmd);
  NetshellError ReadResult(NsCmdResult& cmd_result);

  Errcode Connect(co::net::Endpoint addr);

private:
  AsyncCoro& GetCoro() {
    return coro_;
  }

private:
  const netshell::NsStatusDescriptorTable& status_descriptors_;
  AsyncCoro& coro_;
  io_context& ioc_;
  size_t max_cmd_len_;
  Uptr<co::async::StreamConnector> stm_connector_;
  Uptr<co::async::Stream> stm_;
  Uptr<netshell::NsCommandResultReader> netshell_rdr_;
  Uptr<netshell::NsCommandWriter> netshell_writ_;
  bool connected_{ false };
};

