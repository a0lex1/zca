#include "co/async/server.h"
#include "co/async/service.h"
#include "co/async/wrap_post.h"

#include "co/xlog/xlog.h"

#define llog() Log(_DBG) << "Server " << GET_DEBUG_TAG(_dbg_tag_) << " "
#define llogs() Log(_DBG) << "ServerWithSessList " << GET_DEBUG_TAG(_dbg_tag_) << " "

using namespace std;
using co::net::Endpoint;

namespace co {
namespace async {

const SessionFactoryFunc gEmptySessionFactoryFunc = co::bind(&NullSessionFactoryFn, _1, _2);

ServerObjects::ServerObjects(Shptr<StreamFactory> stm_fac, 
                             Uptr<StreamAcceptor> acpt, 
                             io_context& ioc_sessions_strand,
                             Uptr<StreamAcceptorErrorLogic> errlogic)
  :
  stream_factory(stm_fac), stream_acceptor(move(acpt)),
  ioc_sessions_strand(ioc_sessions_strand),
  errlogic(move(errlogic))
{

}

// ------------------------------------------------------------------------------------------------------------------------------------

DEFINE_XLOGGER_SINK("server", gCoAsyncServerLogSink);
#define XLOG_CURRENT_SINK gCoAsyncServerLogSink

Server::~Server() {
  llog() << "~~~DTOR~~~\n";
}

Server::Server(Endpoint addr, ServerObjects&& objects, SessionFactoryFunc sess_fac_func)
  :
  Fibered(make_shared<Strand>(objects.stream_acceptor->GetIoContext())),
  tss_impl_(make_unique<ThreadsafeStopableImpl>(
              *static_cast<Stopable*>(this),
              GetFiberStrandShptr())),
  addr_(addr),
  objects_(move(objects)),
  sess_fac_func_(sess_fac_func)
{
  llog() << "CTOR\n";

  acpt_nofail_ = make_unique<StreamAcceptorWithErrorLogic>(*objects_.stream_acceptor,
                                                           *objects_.errlogic);
}

Server::Server(Endpoint addr, Shptr<Service> svc, SessionFactoryFunc sess_fac_func)
  :
  Server(addr, ServerObjects(svc->CreateStreamFactory(),
                             svc->CreateStreamAcceptorFactory()->CreateStreamAcceptor(), // ->Shptr
                             svc->GetIoContext()),
         sess_fac_func)
{
}

void Server::Start(RefTracker rt)
{
  DCHECK(bools_.acceptor_setupped_);
  DCHECK(bools_.prepared_to_start_);
  DCHECK(acpt_nofail_->IsOpen());

  tss_impl_->BeforeStartWithIoEnded(rt, rt);

  _dbg_num_accepted_connections_ = 0;
  bools_.stop_accepting_ = false;

  BeginAccept(rt);
}

void Server::StopThreadsafe()
{
  // inside acceptor fiber
  tss_impl_->StopThreadsafe();
}

void Server::PrepareToStart(Errcode& err) {
  DCHECK(!bools_.prepared_to_start_);

  SetupAcceptorNow(err);
  bools_.prepared_to_start_ = true;
}

void Server::CleanupAbortedStop() {
  DCHECK(bools_.prepared_to_start_);
}

bool Server::IsResetSupported() const {
  return false;
}

void Server::ResetToNextRun() {
}

// private
void Server::SetupAcceptorNow(Errcode& err)
{
  if (bools_.acceptor_setupped_) {
    // already setupped, skip
    return;
  }
  DCHECK(!acpt_nofail_->IsOpen());
  acpt_nofail_->Open(err);
  if (err) {
    return;
  }

  llog() << "Binding to " << addr_.ToString() << "\n";

  acpt_nofail_->Bind(addr_, err);
  if (!err) {
    acpt_nofail_->StartListening(err);
    if (!err) {
      bools_.acceptor_setupped_ = true;
      err = NoError();
      return;
    }
  }
  acpt_nofail_->Close();
}

void Server::SetupAcceptorNowNofail()
{
  Errcode err;
  SetupAcceptorNow(err);
  if (err) {
    BOOST_THROW_EXCEPTION(boost::system::system_error(err));
  }
}

// Not required before DTOR
void Server::UnsetupAcceptor()
{
  if (acpt_nofail_->IsOpen()) {
    DCHECK(bools_.acceptor_setupped_);

    acpt_nofail_->Close();
    bools_.acceptor_setupped_ = false;

    DCHECK(!acpt_nofail_->IsOpen());
  }
}

void Server::GetLocalAddressToConnect(Endpoint& addr, Errcode& err) {
  acpt_nofail_->GetLocalAddressToConnect(addr, err);
}

void Server::GetLocalAddress(Endpoint& addr, Errcode& err) {
  acpt_nofail_->GetLocalAddress(addr, err);
}

Endpoint Server::GetLocalAddressToConnect() {
  return acpt_nofail_->GetLocalAddressToConnect();
}

Endpoint Server::GetLocalAddress() {
  return acpt_nofail_->GetLocalAddress();
}

void Server::StartSession(Shptr<SessionBase> sess, RefTracker rt) {
  sess->Start(rt);
}

void Server::BeginAccept(RefTracker rt) {
  DCHECK(bools_.acceptor_setupped_);
  if (bools_.stop_accepting_) {
    return;
  }
  auto new_stream = objects_.stream_factory->CreateStream();
  auto session_strand = make_shared<Strand>(objects_.ioc_sessions_strand);
  next_sess_ = sess_fac_func_(move(new_stream), session_strand);

  EmptyHandler accept_handler = [rt, this]() {
    DCHECK(IsInsideFiberStrand());
    if (bools_.stop_accepting_) { // second check
      return;
    }
    HandleAccept(rt);
  };
  EmptyHandler wrapped_accept_handler = WrapAcceptHandler(accept_handler);

  acpt_nofail_->AsyncAcceptNofail(next_sess_->GetStream(), wrapped_accept_handler);
}

void Server::HandleAccept(RefTracker rt) {

  DCHECK(bools_.acceptor_setupped_); // added, i think it's right
  if (bools_.stop_accepting_) {
    next_sess_->StopThreadsafe();
    return;
  }
  // inside unknown fiber (this func is overloaded in derived)
  ++_dbg_num_accepted_connections_;
  StartSession(next_sess_, rt); //< Derived class can wrap |rt| with its own on_session_stopped
  BeginAccept(rt);
}

EmptyHandler Server::WrapAcceptHandler(EmptyHandler accept_handler) {
  // It seems it's working without wrapping to strand, but we would we want
  // parallel StopUnsafe and HandleAccept? We better wrap it here too like in ServerWithSessList.
  //return accept_handler;
  // Same as in ServerWithSessList

  //
  // TODO: remove override from ServerWithSessList
  //

  return wrap_post(GetFiberStrand(), accept_handler);
}

void Server::StopUnsafe() {
  // in case of ServerWithSesslist, we are in <acceptor fiber> so
  // no worries about synchronization
  bools_.stop_accepting_ = true;
  if (bools_.acceptor_setupped_) {
    objects_.stream_acceptor->CancelAccept();
  }
  UnsetupAcceptor();
  //objects_.GetStreamAcceptor().Close(); // some kind of old way

  if (next_sess_) {
    // Stop Session that is being started. This case is detected
    // when Server::StopThreadsafe() right after Server::Start
    next_sess_->StopThreadsafe();

    next_sess_ = nullptr;
  }
}

#undef XLOG_CURRENT_SINK

// ------------------------------------------------------------------------------------------------------------------------------------

DEFINE_XLOGGER_SINK("listserver", gCoAsyncServerWithSessListLogSink);
#define XLOG_CURRENT_SINK gCoAsyncServerWithSessListLogSink

list<Shptr<Session>>& ServerWithSessList::GetSessionList() {
  return const_cast<list<Shptr<Session>>&>(
    static_cast<const ServerWithSessList&>(*this).GetSessionList());
}

const list<Shptr<Session>>& ServerWithSessList::GetSessionList() const {
  return session_list_;
}

void ServerWithSessList::Start(RefTracker rt) {
  stopping_sessions_ = false;
  Server::Start(rt);
}

void ServerWithSessList::OnSessionStopped(Shptr<Session> sess) {
  DCHECK(IsInsideAcceptorStrand());
  llogs() << " {SESSLIST} removing sess " << SPTR(sess.get()) << "\n";
  session_list_.remove(sess);
  if (events_) {
    events_->OnSessionRemovedFromList(sess);
  }
}

void ServerWithSessList::StartSession(Shptr<SessionBase> sess, RefTracker rt) { // |rt| won't let us destroy {
  // we called from Server::HandleAccept
  DCHECK(IsInsideAcceptorStrand());

  if (stopping_sessions_) {
    // |stopping_sessions_| may be scheduled by StopSessions::AcptLambda; don't start new sessions

    // sess not started, it's safe to StopUnsafe
    //sess->StopUnsafe(); // not yet needed
    return;
  }
  auto our_sess = static_pointer_cast<Session>(sess);
  session_list_.push_back(our_sess);

  auto on_session_stopped = wrap_post(GetFiberStrand(), [&, our_sess]() {

    DCHECK(IsInsideAcceptorStrand());
    // remove session from list
    OnSessionStopped(our_sess);
                               });

  Server::StartSession(sess, RefTracker(CUR_LOC(), on_session_stopped, rt));
}

void ServerWithSessList::StopUnsafe() {
  // inside either strand or io ended/not yet started

  Server::StopUnsafe();

  DoStopSessions();
}

void ServerWithSessList::DoStopSessions() {
  // inside either strand or io ended/not yet started
  if (stopping_sessions_) {
    // Ignore more than one call to StopSessions(). This may be a legacy check.
    return;
  }
  stopping_sessions_ = true;

  llogs() << "stopping " << session_list_.size() << " sessions (inside acceptor fiber)...\n";

  int nstopped = 0;
  for (auto& sess : session_list_) {
    sess->StopThreadsafe();
    nstopped += 1;
    llogs() << nstopped << " sesses stopped - cur is " << SPTR(sess.get()) << "\n";
  }
  llogs() << "Total " << nstopped << " sesses stopped\n";
}


void ServerWithSessList::CleanupAbortedStop() {
  Server::CleanupAbortedStop();

  // If you exited with ioc.stop()

  // As I remember, if we don't do this, there will be a memleak.
  for (auto& sess : session_list_) {
    sess->CleanupAbortedStop();
  }

  if (GetNextSession() && GetNextSession()->IsStarted()) {
    GetNextSession()->CleanupAbortedStop();
  }

  StopUnsafe();
}

bool ServerWithSessList::IsInsideAcceptorStrand() {
  return IsInsideFiberStrand();
}

Strand& ServerWithSessList::GetAcceptorStrand() {
  return GetFiberStrand();
}

EmptyHandler ServerWithSessList::WrapAcceptHandler(EmptyHandler accept_handler) {
  // Same as in Server
  // Synchronize accepting and closing sessions on our
  return wrap_post(GetFiberStrand(), accept_handler);
}

#undef XLOG_CURRENT_SINK


}}


