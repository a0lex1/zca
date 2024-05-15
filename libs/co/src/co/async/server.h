#pragma once

#include "co/async/loop_object.h"

#include "co/async/fibered.h"
#include "co/async/session.h"
#include "co/async/stream_acceptor.h"
#include "co/async/stream_factory.h"

#include "co/base/debug_tag_owner.h"
#include "co/xlog/define_logger_sink.h"

#include <boost/asio/strand.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/deadline_timer.hpp>

namespace co {
namespace async {

DECLARE_XLOGGER_SINK("server", gCoAsyncServerLogSink);
DECLARE_XLOGGER_SINK("listserver", gCoAsyncServerWithSessListLogSink);
DECLARE_XLOGGER_SINK("dbserver", gCoAsyncServerWithDbLogSink);

#define XLOG_CURRENT_SINK gCoAsyncServerLogSink

/*
* class ServerBase
* Sync/async server interface
* 
*/
class ServerBase {
public:
  virtual ~ServerBase() {}

  using Endpoint = net::Endpoint;

  virtual void GetLocalAddress(Endpoint& addr, Errcode& err) = 0;
  virtual void GetLocalAddressToConnect(Endpoint& addr, Errcode& err) = 0;
};

/*
* class ServerObjects
*
*/
class ServerObjects {
public:
  ServerObjects(
    Shptr<StreamFactory> stm_fac,
    Uptr<StreamAcceptor> acpt,
    io_context& ioc_sessions_strand,
    Uptr<StreamAcceptorErrorLogic> errlogic = std::make_unique<StreamAcceptorIgnoreLogic>());

  Shptr<StreamFactory> stream_factory;
  Uptr<StreamAcceptor> stream_acceptor;
  io_context& ioc_sessions_strand;
  Uptr<StreamAcceptorErrorLogic> errlogic;
};

/*
* Session factory func
*
*/
using SessionFactoryFunc = Func<Shptr<Session>(Uptr<Stream> new_stream,
                                               Shptr<Strand> session_strand)>;

static Shptr<Session> NullSessionFactoryFn(Uptr<Stream> new_stream,
                                             Shptr<Strand> session_strand)
{
  class NullSession : public Session, public co::enable_shared_from_this<NullSession> {
  public:
    virtual ~NullSession() = default;
    using Session::Session;
    void BeginIo(RefTracker rt) override { /* just die, session */ }
    void StopUnsafe() override {}
  };
  return make_shared<NullSession>(std::move(new_stream), session_strand);
}

// Use this is you want empty session
extern const SessionFactoryFunc gEmptySessionFactoryFunc;


class Service;

/*
* class Server
*
*/
class Server
  :
  public Fibered,
  public LoopObject,
  public ServerBase,
  private Stopable
{
public:
  virtual ~Server();

  Server(Endpoint addr,
         ServerObjects&& objects,
         SessionFactoryFunc sess_fac_func = SessionFactoryFunc());

  Server(Endpoint addr,
         Shptr<Service> svc,
         SessionFactoryFunc sess_fac_func = SessionFactoryFunc());

  // if not in ctor
  void SetSessionFactoryFunc(SessionFactoryFunc sess_fac_func) { sess_fac_func_ = sess_fac_func; }

  // [ServerBase impl]
  void GetLocalAddress(Endpoint& addr, Errcode& err) override;
  void GetLocalAddressToConnect(Endpoint& addr, Errcode& err) override;

  // [LoopObject impl]
  void PrepareToStart(Errcode&) override;
  void CleanupAbortedStop() override;
  bool IsResetSupported() const override;
  void ResetToNextRun() override;
  void Start(RefTracker rt) override;
  void StopThreadsafe() override;

  // public
  Endpoint GetLocalAddressToConnect();
  Endpoint GetLocalAddress();

  // If you need locaddr before Start(), use this
  void SetupAcceptorNow(Errcode&);
  void SetupAcceptorNowNofail(); // throws

  io_context& GetAcceptorIoContext() { return objects_.stream_acceptor->GetIoContext(); }

  co::DebugTagOwner& _DbgTag() { return _dbg_tag_; }
  uint64_t _DbgNumAcceptedConnections() const { return _dbg_num_accepted_connections_; }

protected:
  Shptr<Session> GetNextSession() { return next_sess_; }

  virtual void StartSession(Shptr<SessionBase> sess, RefTracker rt);

  ServerObjects& GetServerObjects() { return objects_; }

  // [Stopable impl]
  void StopUnsafe() override;

private:
  void BeginAccept(RefTracker rt);
  void HandleAccept(RefTracker rt);

  // Derived class can wrap it with strand to synchronize accepting/removing sessions
  virtual EmptyHandler WrapAcceptHandler(EmptyHandler accept_handler);

  // UnsetupAcceptor is not required before DTOR.
  void UnsetupAcceptor();

private:
  struct {
    bool prepared_to_start_ : 1;
    bool acceptor_setupped_: 1;
    bool stop_accepting_ : 1;
  } bools_ { false, false, false };
  Uptr<ThreadsafeStopableImpl> tss_impl_;
  Endpoint addr_;
  ServerObjects objects_;
  SessionFactoryFunc sess_fac_func_;
  Uptr<StreamAcceptorNofail> acpt_nofail_;
  Shptr<Session> next_sess_;
protected:
  co::DebugTagOwner _dbg_tag_;
  uint32_t _dbg_num_accepted_connections_{0};
};
#undef XLOG_CURRENT_SINK

// --------------------------------------------------------------------------------------------------

#define XLOG_CURRENT_SINK gCoAsyncServerWithSessListLogSink

class ServerWithSessListEvents {
public:
  virtual ~ServerWithSessListEvents() = default;

  virtual void OnSessionRemovedFromList(Shptr<Session> sess) = 0;
};

/*
* class ServerWithSessList
*
*/
class ServerWithSessList : public Server, public SessionListContainer
{
public:
  virtual ~ServerWithSessList() = default;

  using Server::Server;

  void SetEvents(ServerWithSessListEvents& events) {
    events_ = &events;
  }

  // SessionListContainer impl
  std::list<Shptr<Session>>& GetSessionList() override;
  const std::list<Shptr<Session>>& GetSessionList() const;

  // LoopObject override
  void Start(RefTracker rt) override;
  void CleanupAbortedStop() override;
  //IsResetSupported() const 
  //ResetToNextRun()

  bool IsInsideAcceptorStrand(); // sugar

protected:
  void OnSessionStopped(Shptr<Session> sess);

  void StartSession(Shptr<SessionBase> sess, RefTracker rt) override;
  void StopUnsafe() override;

private:
  EmptyHandler WrapAcceptHandler(EmptyHandler accept_handler) override;
  void DoStopSessions();

  Strand& GetAcceptorStrand(); // sugar

private:
  ServerWithSessListEvents* events_{ nullptr };
  std::list<Shptr<Session>> session_list_;
  bool stopping_sessions_{ false };
};
#undef XLOG_CURRENT_SINK


}}




