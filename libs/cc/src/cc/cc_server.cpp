#include "cc/cc_server.h"

#include "co/async/rc4_stream.h"
#include "co/async/hooking_stream_acceptor.h"
#include "co/async/wrap_post.h"
#include "co/xlog/xlog.h"

#include <atomic>

using namespace std;
using namespace co;
using namespace co::async;

#define llog() Log(_DBG) << "CcServ " << SPTR(this) << " "

namespace cc {

DEFINE_XLOGGER_SINK("ccserv", gCcServerSink);
#define XLOG_CURRENT_SINK gCcServerSink

std::atomic<uint64_t> _dbgNumCcServers{0};
std::atomic<uint64_t> _dbgNumCcServersIoStarted{0};
std::atomic<uint64_t> _dbgNumCcServersIoEnded{0};

CcServer::CcServer(Endpoint addr,
                   ServerObjects&& objects,
                   CcServerEvents* ev_disp,
                   uint32_t max_chunk_body_size) // proto messaging
  :
  tss_impl_(*this, make_shared<Strand>(objects.stream_acceptor->GetIoContext())),
  zombiecheck_timer_(objects.stream_acceptor->GetIoContext()),
  addr_(addr),
  ev_disp_(ev_disp),
  max_chunk_body_size_(max_chunk_body_size),
  server_objects_(move(objects))
{
  llog() << "CTOR\n";
  _dbgNumCcServers += 1;
}

CcServer::~CcServer() {
  llog() << "~~~DTOR~~~\n";
  _dbgNumCcServers -= 1;
}

void CcServer::SetPingInterval(time_duration ping_interval) {
  DCHECK(!ping_interval.is_zero());
  ping_interval_ = make_unique<time_duration>(ping_interval);
}

void CcServer::DisablePinging() {
  ping_interval_ = make_unique<time_duration>();
}

void CcServer::SetTrafficEncryptionKeys(
  const void* r_rc4key, size_t r_rc4key_len,
  const void* w_rc4key, size_t w_rc4key_len)
{
  r_rc4key_buffer_ = make_unique<string>(static_cast<const char*>(r_rc4key), r_rc4key_len);
  w_rc4key_buffer_ = make_unique<string>(static_cast<const char*>(w_rc4key), w_rc4key_len);
}

void CcServer::DisableTrafficEncryption() {
  traffic_encryption_enabled_ = false;
}

void CcServer::PrepareToStart(Errcode& err) {
  if (traffic_encryption_enabled_) {
    server_objects_.stream_acceptor = make_unique<HookingStreamAcceptor>(
      move(server_objects_.stream_acceptor));
  }
  server_ = make_unique<ServerWithSessList>(addr_, move(server_objects_),
    co::bind(&CcServer::SessionFactoryFunc, this, _1, _2));
  server_->SetEvents(*this);
  server_->PrepareToStart(err);
  bools_.prepared_to_start = true;
}

void CcServer::Start(RefTracker rt) {
  DCHECK(ping_interval_ != nullptr);
  DCHECK(bools_.prepared_to_start);
  llog() << "Start\n";

  tss_impl_.BeforeStartWithIoEnded(rt, rt);

  _dbgNumCcServersIoStarted += 1;

  RefTracker rt_all(CUR_LOC(),
                    [&] () {
    llog() << "; rt_all\n";
    _dbgNumCcServersIoEnded += 1;
                    },
                    rt);

  server_->Start(rt_all);

  if (!ping_interval_->is_zero()) {
    SetZombieCheckTimer(rt_all);
  }
}

void CcServer::SetZombieCheckTimer(RefTracker rt) {
  DCHECK(ping_interval_ != nullptr);
  // check the bot list a bit more often (3/4) than ping_interval_
  time_duration three_quarter = (*ping_interval_ * 3) / 4;
  zombiecheck_timer_.expires_from_now(three_quarter);
  zombiecheck_timer_.async_wait(
    co::async::wrap_post(server_->GetFiberStrand(),
      co::bind(&CcServer::HandleZombieTimer, this, _1, rt)));
}

void CcServer::HandleZombieTimer(Errcode err, RefTracker rt) {
  DCHECK(server_->IsInsideFiberStrand());

  //#NewFiberShouldCheckForStopping
  if (tss_impl_.InterlockedLoadLastRef() == nullptr) {
    llog() << "HandleZombieTimer: we're stopping\n";
    return;
  }

  if (!err) {
    CheckAndRemoveZombieBots(rt);
    SetZombieCheckTimer(rt);
  }
  else {
    // If error happened, just disable the ping timer
    llog() << "Server Ping check disabled due to error in HandleZombieTimer, err = " << err << "\n";
  }
}

void CcServer::CheckAndRemoveZombieBots(RefTracker rt) {
  DCHECK(server_->IsInsideFiberStrand());
  ExecuteBotListAccessCallback(co::bind(&CcServer::ZombieCheckCallback, this, _1, rt));
}

void CcServer::ZombieCheckCallback(ICcBotList& bot_list, RefTracker rt) {
  DCHECK(server_->IsInsideFiberStrand());
  llog() << "Checking for zombie bots\n";
  for (auto it = bot_list.begin(); it != bot_list.end(); it++) {
    Shptr<ICcBot> bot = *it;
    // is handshake already done by this bot?
    if (bot->GetReadonlyData().GetHandshakeData()) {
      // use x1.5 coefficent, but not less than X seconds
      time_duration _maxtime = (*ping_interval_ * 150) / 100;
      time_duration maxtime;
      auto minimum = boost::posix_time::seconds(1);
      if (_maxtime > minimum) {
        maxtime = _maxtime;
      }
      else {
        maxtime = minimum;
      }
      boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
      if (now - bot->GetReadonlyData().GetLastPingTime() > maxtime) {
        llog() << "Killing ZOMBIE bot " << bot->GetReadonlyData().GetHandshakeData()->GetBotId().ToStringRepr() << "\n";
        bot->Kill();
      }
    }
  }
}

void CcServer::CleanupAbortedStop() {
  server_->CleanupAbortedStop();
}

void CcServer::StopThreadsafe() {
  tss_impl_.StopThreadsafe();
}

void CcServer::StopUnsafe() {
  server_->StopThreadsafe();
  zombiecheck_timer_.cancel();
}

Shptr<Session> CcServer::SessionFactoryFunc(
  Uptr<Stream> new_stream,
  Shptr<Strand> session_strand)
{
  if (traffic_encryption_enabled_) {
    DCHECK(r_rc4key_buffer_ != nullptr);
    DCHECK(w_rc4key_buffer_ != nullptr);
    new_stream = make_unique<Rc4Stream>(
      move(new_stream),
      r_rc4key_buffer_->c_str(), r_rc4key_buffer_->length(),
      w_rc4key_buffer_->c_str(), w_rc4key_buffer_->length());
  }
  auto new_sess = make_shared<CcServerSession>(
    move(new_stream),
    session_strand,
    max_chunk_body_size_,
    ev_disp_,
    *ping_interval_);

  // sessions need to know us, because they query the bot list
  new_sess->SetOwnerServer(*this);

  return new_sess;
}

void CcServer::ExecuteBotListAccessCallback(Func<void(ICcBotList&)> cbk) {
  boost::asio::dispatch(server_->GetFiberStrand(), [&, cbk]() {
    // inside acceptor fiber, call user |cbk|
    cbk(*this);
                    });
}

// [ICcBotList impl]
CcBotRecordIterator CcServer::begin() {
  SessionListContainer::Iterator list_it(server_->GetSessionList().begin());
  return CcBotRecordIterator(move(list_it));
}

CcBotRecordIterator CcServer::end() {
  SessionListContainer::Iterator list_it(server_->GetSessionList().end());
  return CcBotRecordIterator(move(list_it));
}

size_t CcServer::GetCount() const {
  return server_->GetSessionList().size();
}

void CcServer::OnSessionRemovedFromList(Shptr<Session> sess) {
  if (ev_disp_) {
    auto downcast(std::static_pointer_cast<CcServerSession>(sess));
    auto upcast(std::static_pointer_cast<ICcBot>(downcast));
    ev_disp_->OnBotRemovedFromList(downcast);
  }
}

co::net::Endpoint CcServer::GetLocalAddress() {
  Endpoint addr;
  Errcode err;
  GetLocalAddress(addr, err);
  if (err) {
    BOOST_THROW_EXCEPTION(boost::system::system_error(err));
  }
  return addr;
}

co::net::Endpoint CcServer::GetLocalAddressToConnect() {
  Endpoint addr;
  Errcode err;
  GetLocalAddressToConnect(addr, err);
  if (err) {
    BOOST_THROW_EXCEPTION(boost::system::system_error(err));
  }
  return addr;
}


}


