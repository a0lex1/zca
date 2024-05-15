#pragma once

#include "co/async/cleanupable.h"
#include "co/async/threadsafe_stopable_impl.h"
#include "co/async/startable_stopable.h"
#include "co/async/stream.h"
#include "co/async/fibered.h"
#include "co/base/debug_tag_owner.h"
#include "co/xlog/define_logger_sink.h"

#include <boost/asio/strand.hpp>

namespace co {
namespace async {

DECLARE_XLOGGER_SINK("sess", gCoAsyncSessSink);

class Client;
class Server;

class SessionBase
  :
  public Startable,
  protected Stopable,
  public DebugTagOwner,
  public Cleanupable
{
public:
  virtual ~SessionBase();

  SessionBase(Uptr<Stream> stm);

  void Start(RefTracker rt) override;

  // public, hidden in Session
  void StopUnsafe() override;

  // Cleanupable impl
  void CleanupAbortedStop() override { cleaned_ = true; }

  bool IsStarted() const { return started_; }
  bool IsStopped() const { return stopped_; }
  bool IsCleaned() const { return cleaned_; }

protected:
  Stream& GetStream();

private:
  // To implement by user
  virtual void BeginIo(RefTracker rt) = 0;

  friend class Client;
  friend class Server;

private:
  //struct {
    bool started_ : 1;
    bool stopped_ : 1;
    bool cleaned_ : 1;

    bool _dbg_closed_ : 1;
    bool _dbg_dtor_ : 1;
  //} bools_ { false, false, false, false, false };

  Uptr<Stream> stream_;

  size_t _dbg_stopunsafe_called_times_{0};
  const char* _dbg_tagwas_{nullptr};
};

// --------------------------------------------------------------------------------------------------------------

class Client;
class ServerWithSessList;

class Session: public SessionBase, public ThreadsafeStopable, public Fibered
{
public:
  virtual ~Session() = default;

  Session(Uptr<Stream> new_stm, Shptr<Strand> strand);

  void Start(RefTracker rt) final;

  // may be called before Start()
  void StopThreadsafe() override;

protected:
  // Only StreamIo is accessible for derived classes.
  StreamIo& GetStreamIo();

  // For example, you can access GetThreadsafeStopableImpl().InterlockedLoadLastRef
  ThreadsafeStopableImpl& GetThreadsafeStopableImpl() {return tss_impl_; }

  using SessionBase::StopUnsafe;

private:
  // UPD: unhidden cuz derived classes need it
  // Hide stream's Close/Shutdown functionality.
  // Now the single point of stop is our Stop()
  //using SessionBase::GetStream;
  //void OnIoEnded() override; // not needed anymore

  friend class Client;
  friend class ServerWithSessList;

  ThreadsafeStopableImpl tss_impl_;
};

// ------------------------------------------------------------------------------------------------------------------------

class SessionListContainer {
public:
  virtual ~SessionListContainer() = default;

  using Iterator = std::list<Shptr<Session>>::iterator;
  using ConstIterator = std::list<Shptr<Session>>::const_iterator;

  virtual std::list<Shptr<Session>>& GetSessionList() = 0;
};

template <typename T>
bool operator == (const Weakptr<T>& a, const Weakptr<T>& b) {
  return a.lock() == b.lock();
}


}}
