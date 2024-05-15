#include "cc/cc_client.h"

//#include "co/async/create_for_endpoint.h"
#include "co/async/rc4_stream.h"
#include "co/async/hooking_stream_connector.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;

#define llog() Log(_DBG) << "CcCli " << SPTR(this) << " "

namespace cc {

DEFINE_XLOGGER_SINK("cccli", gCcClientSink);
#define XLOG_CURRENT_SINK gCcClientSink

void CcClient::SetRemoteAddress(const Endpoint& remaddr) {
  // Before refactoring, this function can only be called *before* PrepareToStart()
  // because it was utilizing cli_->SetRemoteAddress, not cached remaddr_
  if (cli_) {
    cli_->SetRemoteAddress(remaddr);
  }
  remaddr_ = remaddr;
}

void CcClient::SetPingInterval(boost::posix_time::time_duration ping_interval) {
  DCHECK(!ping_interval.is_zero());
  shared_data_->ping_interval_ = make_unique<time_duration>(ping_interval);
}

void CcClient::UseServerPingInterval() {
  shared_data_->ping_interval_ = make_unique<time_duration>();
}

void CcClient::SetTrafficEncryptionKeys(
  const void* r_rc4key, size_t r_rc4key_len,
  const void* w_rc4key, size_t w_rc4key_len)
{
  r_rc4key_buffer_ = make_unique<string>(static_cast<const char*>(r_rc4key), r_rc4key_len);
  w_rc4key_buffer_ = make_unique<string>(static_cast<const char*>(w_rc4key), w_rc4key_len);
}

void CcClient::DisableTrafficEncryption() {
  traffic_encryption_enabled_ = false;
}

void CcClient::SetOpaqueHandshakeData(Uptr<string> opaque_hshake_data) {
  opaque_hshake_data_ = move(opaque_hshake_data);
}

void CcClient::PrepareToStart(Errcode& err) {
  Uptr<Stream> new_stm = std::move(stm_);
  Shptr<StreamConnector> connector = connector_;

  if (traffic_encryption_enabled_) {
    DCHECK(r_rc4key_buffer_ != nullptr);
    DCHECK(w_rc4key_buffer_ != nullptr);
    new_stm = make_unique<Rc4Stream>(
      move(new_stm),
      r_rc4key_buffer_->c_str(), r_rc4key_buffer_->length(),
      w_rc4key_buffer_->c_str(), w_rc4key_buffer_->length());

    connector = make_unique<HookingStreamConnector>(*connector_);
  }

  auto session = make_shared<CcClientSession>(
    move(new_stm),
    tss_impl_.GetStrandShptr(),
    command_dispatcher_, events_, max_chunk_body_size_, shared_data_,
    move(opaque_hshake_data_));

  cli_ = make_unique<Client>(remaddr_, connector, session);
  cli_->PrepareToStart(err);
  bools_.prepared_to_start = true;
}

void CcClient::CleanupAbortedStop() {
  cli_->CleanupAbortedStop();
}

void CcClient::Start(RefTracker rt) {
  DCHECK(shared_data_->ping_interval_ != nullptr);
  DCHECK(bools_.prepared_to_start);

  tss_impl_.BeforeStartWithIoEnded(rt, rt);

  cli_->Start(rt);
}

void CcClient::StopThreadsafe() {
  tss_impl_.StopThreadsafe();
}

const std::vector<CcError>& CcClient::GetLastErrorStack() const {
  return shared_data_->err_stack_;
}

Errcode CcClient::GetConnectError() const {
  return cli_->GetConnectError();
}

void CcClient::StopUnsafe() {
  cli_->StopThreadsafe();
}


}



