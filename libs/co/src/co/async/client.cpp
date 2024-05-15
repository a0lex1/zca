#include "co/async/client.h"

#include "co/async/wrap_post.h"

#include "co/xlog/xlog.h"

using namespace std;

#define llog() Log(_DBG) << "Client " << SPTR(this) << " "

namespace co {
namespace async {

DEFINE_XLOGGER_SINK("client", gCoAsyncClientLogSink);
#define XLOG_CURRENT_SINK gCoAsyncClientLogSink

Client::~Client() {
  llog() << "~~~DTOR~~~\n";
}

Client::Client(Endpoint addr,
               Shptr<StreamConnector> connector,
               Shptr<Session> client_session)
  :
    tss_impl_(*this, client_session->GetFiberStrandShptr()),
    addr_(addr),
    client_session_(client_session), // Already has Stream
    connector_(connector)
{
  bools_.addr_set_ = true;
}

void Client::PrepareToStart(Errcode& err) {
  DCHECK(bools_.addr_set_);
  _dbg_preparetostart_called_ += 1;
  bools_.prepared_to_start_ = true;
}

void Client::Start(RefTracker rt) {
  DCHECK(!bools_.started_);
  DCHECK(bools_.prepared_to_start_);
  DCHECK(bools_.addr_set_);

  tss_impl_.BeforeStartWithIoEnded(rt, rt);

  _dbg_start_called_ += 1;

  //bools_.stop_ = false;
  bools_.started_ = true;

  RefTracker rt_all(CUR_LOC(),
                    [&] () {
    llog() << "; rt_all {sess `" << GET_DEBUG_TAG(*client_session_.get()) << "`}\n";
    client_session_ = nullptr;

  }, rt);

  connector_->AsyncConnect(addr_, client_session_->GetStream(),
    wrap_post(GetSessionStrand(),
              co::bind(&Client::HandleConnect, this, _1, rt_all)));
}

void Client::StopThreadsafe() {
  _dbg_stopthreadsafe_called_ += 1;

  tss_impl_.StopThreadsafe();
}

void Client::SetRemoteAddress(const Endpoint &remaddr) {
  addr_ = remaddr;
  bools_.addr_set_ = true;
}

void Client::StopUnsafe() {
  DCHECK(bools_.started_);
  // Either strand or i/o not started/ended
  if (!bools_.stop_) {
    // Do real stop one time. |client_session_| may have been already deleted
    // if rt_all io ended

    bools_.stop_ = true;
    if (client_session_) {
      client_session_->StopThreadsafe();
    }
  }
}

bool Client::IsAddressSet() const {
  return bools_.addr_set_;
}

void Client::CleanupAbortedStop() {
  if (client_session_) {
    client_session_->CleanupAbortedStop();
  }
}

bool Client::IsResetSupported() const {
  // we can't create new session, we have only one session supplied to CTOR
  return false;
}

void Client::ResetToNextRun() {
  // nothing
  _dbg_resettonextrun_called_ += 1;
}

void Client::HandleConnect(Errcode err, RefTracker rt) {
  DCHECK(InsideSessionStrand());
  _dbg_handleconnect_called_ += 1;
  if (err) {
    // User should check GetConnectError() first.
    // If it's OK (0), user can use RefTracker's error,
    // They both can be connection_failed. But the first one
    // means can't connect, second one means anything else.
    conn_err_ = err;
    return;
  }
  if (bools_.stop_) {
    // |bools_.stop_| is set in StopUnsafe()
    return;
  }
  RefTracker rt_sess(CUR_LOC(),
                     [&] () {
    llog() << " ; rt_sess {sess `" << GET_DEBUG_TAG(*client_session_.get()) << "`}\n";
  }, rt);

  DCHECK(!client_session_->_dbg_stopunsafe_called_times_);

  client_session_->Start(rt_sess);
}


}
}


