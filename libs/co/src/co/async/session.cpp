#include "co/async/session.h"
#include "co/base/strings.h"
#include "co/xlog/xlog.h"

#include <atomic>
#include <boost/make_shared.hpp>

#define llogt() Log(_TRACE) << "SessionBase (" << SPTR(this) << ") " << GET_DEBUG_TAG(*this) << " "
#define llogb() Log(_DBG) << "SessionBase (" << SPTR(this) << ") " << GET_DEBUG_TAG(*this) << " "

using namespace std;

namespace co {
namespace async {

DEFINE_XLOGGER_SINK("sess", gCoAsyncSessSink);
#define XLOG_CURRENT_SINK gCoAsyncSessSink

SessionBase::SessionBase(Uptr<Stream> stm) : stream_(move(stm)) {
  llogt() << "CTOR\n";

  started_ = stopped_ = cleaned_ = false;

  _dbg_closed_ = _dbg_dtor_ = false;
}
  
SessionBase::~SessionBase() {
 llogt() << "~~~DTOR~~~\n";

  DCHECK(!_dbg_dtor_);

  _dbg_dtor_ = true;
}

co::async::Stream& SessionBase::GetStream() {
  return *stream_.get();
}

void SessionBase::Start(RefTracker rt) {
  DCHECK(!started_);

  //DCHECK(0 == _dbg_stopunsafe_called_times_); // allow StopUnsafe before Start
  DCHECK(!_dbg_closed_);

  started_ = true;

  // Call user's 'begin i/o' entry point.
  BeginIo(rt);
}

void SessionBase::StopUnsafe() {
  DCHECK(IsStarted());

  if (_dbg_dtor_) {
    llogb() << "!*!*!*!**!*! UAFED (SessionBase " << SPTR(this) << ", tag `" << GET_DEBUG_TAG(*this) << "`)\n";
  }
  //DCHECK(!_dbg_dtor_);

  _dbg_stopunsafe_called_times_ += 1;

  Errcode err_ign;
  if (stream_->IsOpen()) {

    // Not needed here. The default is to close the Session.
    // If you want to shutdown, do it before this.
    //stream_->Shutdown(err_ign);

    llogt() << "StopUnsafe: Stream is open, closing it once\n";

    stream_->Close();
    _dbg_closed_ = true;
  }
  else {
    llogt() << "StopUnsafe: stream already closed\n";
  }

  stopped_ = true;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------

Session::Session(Uptr<Stream> new_stm, Shptr<Strand> strand)
  :
  SessionBase(move(new_stm)),
  Fibered(strand),
  tss_impl_(*this, GetFiberStrandShptr())
{
}

StreamIo& Session::GetStreamIo() {

  return GetStream();
}

void Session::Start(RefTracker rt) {

  // tss automatically adds tss.OnIoEnded() call
  RefTracker rt_new;
  tss_impl_.BeforeStartWithIoEnded(rt, rt_new);

  SessionBase::Start(rt_new);
}

void Session::StopThreadsafe() {

  tss_impl_.StopThreadsafe();
}


}}


