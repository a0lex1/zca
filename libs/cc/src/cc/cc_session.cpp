#include "cc/cc_session.h"

#include "co/async/wrap_post.h"

#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;

#define llogt() Log(_TRACE) << "CcSess (base)" << SPTR(static_cast<co::async::Session*>(this)) << " "
#define llogd() Log(_DBG) << "CcSess (base)" << SPTR(static_cast<co::async::Session*>(this)) << " "

namespace cc {

DEFINE_XLOGGER_SINK("ccsess", gCcSessionSink);
#define XLOG_CURRENT_SINK gCcSessionSink

CcSession::~CcSession() {
  llogt() << "~~~DTOR~~~\n";
}

CcSession::CcSession(Uptr<Stream> new_stm,
                     Shptr<Strand> strand,
                     uint32_t max_chunk_body_size)
  :
  Session(std::move(new_stm), strand),
  chunk_writer_(GetFiberStrandShptr(), GetStream()),
  chunk_writer_qst_(chunk_writer_), // uses underlying_ strand
  chunk_reader_(GetFiberStrandShptr(), GetStream(), max_chunk_body_size),
  msg_writer_(chunk_writer_qst_),
  msg_reader_(chunk_reader_, cc_msg_fac_)
{
  llogt() << "CTOR\n";
}

void CcSession::StopUnsafe() {
  // May be inside some post operation
  stopping_ = true;

  // inside any fiber (io may be ended in any fiber, if we're
  // here from OnIoEnded, not from StopThreadsafe

  llogt() << "StopUnsafe, calling Session::StopUnsafe\n";
  Session::StopUnsafe();
}

void CcSession::PushLastError(const CcError& ccerr) {
  GetLastErrorStack().push_back(ccerr);
}

void CcSession::ReadNextMessage(RefTracker rt) {
  msg_reader_.AsyncReadMessage(cur_read_msg_,
                               wrap_post(GetFiberStrand(),
                               co::bind(&CcSession::HandleReadMessage,
                                        GetSharedFromThisAs<>(), _1, rt)));
}

void CcSession::HandleReadMessage(ProtoError pt_err, RefTracker rt)   {
  DCHECK(IsInsideFiberStrand());
  if (pt_err) {
    llogd() << "HandleReadMessage PT_ERROR " << pt_err.MakeErrorMessage() << "\n";
    PushLastError(CcError(CcErrc::proto_read_error, CcErrorInfo(pt_err)));
    StopThreadsafe();
    return;
  }
  DispatchProtoMessage(*cur_read_msg_.get(),
                       wrap_post(GetFiberStrand(),
                         co::bind(&CcSession::HandleDispatchMessage,
                                  GetSharedFromThisAs<>(), _1, rt)));
}

void CcSession::HandleDispatchMessage(const CcError& ccerr, RefTracker rt) {
  DCHECK(IsInsideFiberStrand());
  if (ccerr) {
    llogd() << "HandleDispatchMessage ERROR " << ccerr.MakeErrorMessage() << "\n";
    PushLastError(ccerr);
    StopThreadsafe();
    return;
  }
  ReadNextMessage(rt);
}

// ---

void CcSession::StartReadMessageLoop(RefTracker rt) {
  ReadNextMessage(rt);
}

io_context& CcSession::GetIoContext()
{
  // needed since we hidden GetStream()
  return GetStream().GetIoContext();
}

}


