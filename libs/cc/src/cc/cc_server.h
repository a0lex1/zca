#pragma once

#include "cc/cc_server_session.h"

#include "co/async/server.h"
#include "co/async/threadsafe_stopable_impl.h"
#include "co/async/loop_object.h"
#include "co/xlog/define_logger_sink.h"

#include <atomic>

namespace cc {

DECLARE_XLOGGER_SINK("ccserv", gCcServerSink);

extern std::atomic<uint64_t> _dbgNumCcServers;
extern std::atomic<uint64_t> _dbgNumCcServersIoStarted;
extern std::atomic<uint64_t> _dbgNumCcServersIoEnded;

/***********************************************************************
* class CcServerBase
* 
************************************************************************/

class CcServerBase {
public:  
  virtual ~CcServerBase() = default;

  /* thread-safe */  
  virtual void ExecuteBotListAccessCallback(Func<void(ICcBotList&)>  cbk) = 0;

  //virtual void GetBotListSnapshotWeak(Uptr<ICcBotListSnapWeak>& blw, EmptyHandler handler) = 0;
  //virtual void GetBotListSnapshotStrong(Uptr<ICcBotListSnapStrong>& bl, EmptyHandler handler) = 0;
};

/***********************************************************************
* class CcServer
* 
************************************************************************/
class CcServer
  :
  public co::async::LoopObjectNoreset,
  private co::async::Stopable,
  public CcServerBase,
  private ICcBotList,
  private co::async::ServerWithSessListEvents
{
public:
  virtual ~CcServer();

  using RefTracker = co::RefTracker;
  using Endpoint = co::net::Endpoint;
  using StreamFactory = co::async::StreamFactory;
  using StreamAcceptor = co::async::StreamAcceptor;
  using StreamAcceptorErrorLogic = co::async::StreamAcceptorErrorLogic;
  using StreamAcceptorIgnoreLogic = co::async::StreamAcceptorIgnoreLogic;
  using ServerWithSessList = co::async::ServerWithSessList;
  using ServerObjects = co::async::ServerObjects;
  using Session = co::async::Session;
  using Stream = co::async::Stream;
  using ThreadsafeStopableImpl = co::async::ThreadsafeStopableImpl;
  using deadline_timer = boost::asio::deadline_timer;
  using time_duration = boost::posix_time::time_duration;

  CcServer(
    Endpoint addr,
    ServerObjects&& objects,
    CcServerEvents* ev_disp,
    uint32_t max_chunk_body_size // proto messaging
    );

  // Should be either set or disabled
  void SetPingInterval(time_duration ping_interval);
  void DisablePinging();

  // Traffic encryption keys need to be either set or encryption needs to be disabled
  void SetTrafficEncryptionKeys(const void* r_rc4key, size_t, const void* w_rc4key, size_t);
  void DisableTrafficEncryption();

  void PrepareToStart(Errcode& err) override;

  void Start(RefTracker rt) override;

  void CleanupAbortedStop() override;

  void StopThreadsafe() override;

  // [CcServerBase impl]
  void ExecuteBotListAccessCallback(Func<void(ICcBotList&)>  cbk) override;
  //void GetBotListSnapshotWeak(Uptr<ICcBotListSnapWeak>& blw, EmptyHandler handler) override;
  //void GetBotListSnapshotStrong(Uptr<ICcBotListSnapStrong>& bl, EmptyHandler handler) override;

  void GetLocalAddress(Endpoint& addr, Errcode& err) { return server_->GetLocalAddress(addr, err); }
  void GetLocalAddressToConnect(Endpoint& addr, Errcode& err) { return server_->GetLocalAddressToConnect(addr, err); }

  Endpoint GetLocalAddress();
  Endpoint GetLocalAddressToConnect();

private:
  void StopUnsafe() override;

  // for |bkg_checker_|
  //void CheckBot(Shptr<co::async::Session> bot_sess) { //  auto our_sess = std::static_pointer_cast<CcServerSession>(bot_sess);  //  our_sess->DoBackgroundCheck();  //}

  // [ICcBotList impl]
  CcBotRecordIterator begin() override;
  CcBotRecordIterator end() override;
  size_t GetCount() const override;

  // [ServerWithSessListEvents impl]
  void OnSessionRemovedFromList(Shptr<Session> sess) override;

  Shptr<Session> SessionFactoryFunc(Uptr<Stream>, Shptr<Strand>);
  void SetZombieCheckTimer(RefTracker);
  void HandleZombieTimer(Errcode, RefTracker);
  void CheckAndRemoveZombieBots(RefTracker);
  void ZombieCheckCallback(ICcBotList&, RefTracker rt);

  friend class CcServerSession;
  bool IsInsideAcceptorStrand() { return server_->IsInsideAcceptorStrand(); }
  
private:
  ThreadsafeStopableImpl tss_impl_;

  Endpoint addr_;
  Uptr<ServerWithSessList> server_;

  CcServerEvents* ev_disp_;
  uint32_t max_chunk_body_size_;

  Uptr<time_duration> ping_interval_;
  deadline_timer zombiecheck_timer_;

  bool traffic_encryption_enabled_{ true };
  Uptr<std::string> r_rc4key_buffer_;
  Uptr<std::string> w_rc4key_buffer_;

  struct {
    bool prepared_to_start : 1;
  } bools_{ false };

  ServerObjects server_objects_; // should go last before we're moving them
};


}


