// TODO: REMOVE THIS CODE, use netshell/sync

#include "zca/sync/sync_netshell_client.h"

using namespace std;
using namespace co;
using namespace co::async;
using co::net::Endpoint;
using namespace netshell;

SyncNetshellClient::SyncNetshellClient(
  const netshell::NsStatusDescriptorTable& status_descriptors,
  AsyncCoro& coro,
  io_context& ioc, size_t max_cmd_len)
  :
  status_descriptors_(status_descriptors),
  coro_(coro),
  ioc_(ioc),
  max_cmd_len_(max_cmd_len),
  connected_(false)
{
}

Errcode SyncNetshellClient::Disconnect() {
  DCHECK(connected_);
  Errcode ignored_err;
  stm_->Shutdown(ignored_err); // !
  stm_->Close();
  stm_connector_ = nullptr;
  stm_ = nullptr;
  connected_ = false;
  return co::NoError();
}

NetshellError SyncNetshellClient::ExecuteCommand(const std::string& cmd,
                                                 NsCmdResult& cmd_result) {
  NetshellError ns_err_;
  ns_err_ = WriteCommand(cmd);
  if (ns_err_) {
    return ns_err_;
  }
  ns_err_ = ReadResult(cmd_result);
  return ns_err_;
}

// ---

NetshellError SyncNetshellClient::WriteCommand(const std::string& cmd) {
  DCHECK(connected_);
  auto cbk(GetCoro().CreateContinuationCallback<NetshellError>());
  // |cbk| exists in coro's stack during DoYield()
  GetCoro().DoYield([&, cmd]() { // |cmd| captured by copy, ok
    // new_io callback. Executed right after coroutine yielded to make new i/o that will reenter
    // We're gonna grab |cbk| from frozen coro's stack. Tested that way, works.
    // Also tested with a copy of |cbk| [ captured ].
    netshell_writ_->AsyncWriteCommand(cmd, cbk.GetFunction());
                    });
  // I/O returned.
  return std::get<0>(cbk.ResultTuple());
}

NetshellError SyncNetshellClient::ReadResult(NsCmdResult& cmd_result) {
  DCHECK(connected_);
  auto cbk(GetCoro().CreateContinuationCallback<NetshellError>());
  GetCoro().DoYield([&]() {
    // new_io callback
    // Executed after coroutine yielded.
    netshell_rdr_->AsyncReadResult(cmd_result, cbk.GetFunction());
                    });
  // I/O returned.
  return std::get<0>(cbk.ResultTuple());
}

Errcode SyncNetshellClient::Connect(Endpoint addr) {
  DCHECK(!connected_);

  stm_ = CreateStreamForEndpoint(ioc_, addr);
  DCHECK(stm_ != nullptr);
  stm_connector_ = CreateConnectorForEndpoint(addr);
  DCHECK(stm_connector_ != nullptr);

  netshell_rdr_ = make_unique<NsCommandResultReaderText>(
        status_descriptors_,
        make_shared<Strand>(ioc_), // strand 1
        *stm_.get(), max_cmd_len_); // using TEXT, no selection

  netshell_writ_ = make_unique<NsCommandWriterText>(
        make_shared<Strand>(ioc_), // strand 2
        *stm_.get());

  auto cbk = GetCoro().CreateContinuationCallback<Errcode>();
  GetCoro().DoYield([&, addr]() {
    // new_io callback.
    // Executed after coroutine yielded.
    stm_connector_->AsyncConnect(addr, *stm_.get(), cbk.GetFunction());
                    });
  if (std::get<0>(cbk.ResultTuple()) != co::NoError()) {
    // Connection failed
    stm_connector_ = nullptr;
    stm_ = nullptr;
    return std::get<0>(cbk.ResultTuple());
  }
  // Connection succeeded
  connected_ = true;
  return co::NoError();
}
