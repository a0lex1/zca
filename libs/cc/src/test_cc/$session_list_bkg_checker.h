class SessionListBkgChecker : public Startable, private Stopable {
public:
  typedef Func<void(Shptr<StopableSession>)> CheckerFunc;

  virtual ~SessionListBkgChecker() = default;
  
  SessionListBkgChecker(boost::asio::executor post_to,
                        SessionListContainer& list_container,
                        CheckerFunc check_func);
    
  void Start() override = 0; //emphasize
  void Stop() override = 0; //emphasize
private:
  SessionListContainer& list_container_;
  CheckerFunc check_func_;
);

class SessionListBkgCheckerBasic : public SessionListBkgChecker {
public:
  typedef Func<void(Shptr<StopableSession>)> CheckerFunc;

  virtual ~SessionListBkgCheckerBasic() = default;
  
  SessionListBkgCheckerBasic(boost::asio::executor post_to,
                             SessionListContainer& list_container,
                             CheckerFunc check_func,
                             io_context& ioc,
                             boost::posix_time::time_duration check_interval)
    :
    SessionListBkgChecker(post_to, list_container,  check_func),
    timer_(ioc),
    check_interval_(check_interval)
  {    
  }
  virtual void Start(RefTracker rt) override {
    timer_
  }
  virtual void Stop() override {
  }
private:
  SessionListContainer& list_container_;
  CheckerFunc check_func_;
  boost::posix_time::time_duration check_interval_;
  boost::asio::deadline_timer timer_;
)

//----------------------------------------------------------------

class SessionListBkgCheckerBalanced : public SessionListBkgChecker {
public:
  virtual ~SessionListBkgCheckerBalanced() = default;

  typedef Func<void(Shptr<StopableSession>)> CheckerFunc;
  
  SessionListBkgCheckerBalanced(SessionListContainer& list_container, CheckerFunc check_func,
    size_t piece_session_count,
    boost::posix_time::time_duration piece_time_interval);
    
  void Start() {
  }
  Stop() override {
  }
  
  .... <<<<<<<<<<<<<<<<<
  
)
  
  
  
}

//=======> this could be separate SessionListChecker/SessionListBkgchecker
//
//-----> write it from this draft:


#pragma once

#include "co/async/server.h"

... >>>>>>>>>>>>>>

class ServerWithSessListChecker: public ServerWithSessList {
public:
  ServerWithSessListChecker(
    Func<void(Shptr<StopableSession>)> check_func,
    size_t piece_session_count,
    boost::posix_time::time_duration piece_time_interval,
    Endpoint addr,
    stopable_session_factory_func sess_fac_func,
    Shptr<StreamFactory> stm_fac,
    Uptr<StreamAcceptor> acpt,
    Uptr<StreamAcceptorErrorLogic> errlogic = std::make_unique<StreamAcceptorIgnoreLogic>())
    :
    ServerWithSessList(addr, sess_fac_func, stm_fac, std::move(acpt), std::move(errlogic)),
    check_func_(check_func),
    piece_session_count_(piece_session_count), // -1
    piece_time_interval_(piece_time_interval), // 5000msec
    timer_(GetAcceptorIoContext())
  {
  }
  virtual void Start(RefTracker rt) override {
    cur_sess_iterator_ = GetSessionList().begin();
    RestartTimer(rt);
    ServerWithSessList::Start(rt);
  }
  virtual void Stop() override {
    timer_.cancel();
  }
private:
  void RestartTimer(RefTracker rt) {
    timer_.expires_from_now(piece_time_interval_);
    timer_.async_wait(bind_executor(GetAcceptorStrand(), &ServerWithSessListChecker::HandleWait, _1, rt));
  }
  void HandleWait(Errcode err, RefTracker rt) {
    Log(_DBG) << "checking\n";
    CheckNextPiece();
    Log(_DBG) << "check done, restarting timer\n";
    RestartTimer();
  }
  void CheckNextPiece(RefTracker rt) {
    /***** NOT GOOD ******/
    bool end_met = false;
    size_t processed = 0;
  check_more:
    Log(_DBG) << "checking more\n";
    while (cur_sess_iterator_ != GetSessionList().end()) {
      if (piece_session_count_ != -1) {
        if (processed == piece_session_count_) {
          Log(_DBG) << "all sessions in piece were checked\n";
          return;
        }
      }
      check_func_(*cur_sess_iterator_);
      cur_sess_iterator_++;
      processed++;
    }
    // If we get here, the end is reached
    DCHECK(cur_sess_iterator_ == GetSessinList.end());
    if (end_met) {
      Log(_DBG) << "second time end reached, piece>list\n";
      return;
    }
    Log(_DBG) << "end reached, starting from begin()\n";
    end_met = true;
    cur_sess_iterator_ = GetSessionList.begin();
    goto check_more;
  }
private:
  virtual void OnSessionStopped(Shptr<StopableSession> our_sess) override {
    // INSIDE ACCEPTOR FIBER
    if (*cur_sess_iterator_ == our_sess) {
      if (cur_sess_iterator_ == GetSessinList().begin()) {
        // removing the only single entry, list is gonna be empty
        cur_sess_iterator_ = GetSessinList().end());
        Log(_DBG) << "removing the only entry which is !current!\n";
      }
      else {
        // Have space to move backwards
        cur_sess_iterator_ --;
        Log(_DBG) << "removing !current! entry, shifting back\n";
      }
    }
    ServerWithSessList::OnSessionStopped(our_sess);
  }
private:
  void(Shptr<StopableSession>) check_func_;
  size_t piece_session_count_;
  boost::posix_time::time_duration piece_time_interval_;
  boost::asio::deadline_timer timer_;
  std::list<Shptr<StopableSession>>::iterator cur_sess_iterator_;
};





