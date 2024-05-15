#include "co/async/stream_acceptor.h"
#include "co/xlog/xlog.h"

namespace co {
namespace async {

DEFINE_XLOGGER_SINK("acceptor", gCoAsyncStreamAcceptorLogSink);
#define XLOG_CURRENT_SINK gCoAsyncStreamAcceptorLogSink

bool StreamAcceptorIgnoreLogic::OnError(Errcode err) {
  // Test
  //Log(_DBG) << "Accept error: " << err << "\n";
  // issue another accept
  return kIssueAnotherAccept;
}

StreamAcceptorWithErrorLogic::StreamAcceptorWithErrorLogic(
  StreamAcceptor& acpt_with_fail, StreamAcceptorErrorLogic& err_logic)
  :
  acpt_with_fail_(acpt_with_fail), err_logic_(err_logic)
{
}

void StreamAcceptorWithErrorLogic::AsyncAcceptNofail(
  Stream& stm,
  AcceptHandler user_handler)
{
  auto h = bind(&StreamAcceptorWithErrorLogic::HandleAcceptConsultErrorLogic,
    this, _1, std::ref(stm), user_handler);

  acpt_with_fail_.AsyncAccept(
    stm,
    [=] (Errcode err) {
      h(err);
    }
  );
}

bool StreamAcceptorWithErrorLogic::IsOpen() {
  return acpt_with_fail_.IsOpen();
}

void StreamAcceptorWithErrorLogic::Open(Errcode& err) {
  acpt_with_fail_.Open(err);
}

void StreamAcceptorWithErrorLogic::Bind(Endpoint addr, Errcode& err) {
  acpt_with_fail_.Bind(addr, err);
}

void StreamAcceptorWithErrorLogic::StartListening(Errcode& err) {
  acpt_with_fail_.StartListening(err);
}

void StreamAcceptorWithErrorLogic::GetLocalAddress(Endpoint& addr, Errcode& err) {
  acpt_with_fail_.GetLocalAddress(addr, err);
}

void StreamAcceptorWithErrorLogic::GetLocalAddressToConnect(Endpoint& addr, Errcode& err) {
  acpt_with_fail_.GetLocalAddressToConnect(addr, err);
}

void StreamAcceptorWithErrorLogic::Close() {
  acpt_with_fail_.Close();
}

void StreamAcceptorWithErrorLogic::CancelAccept(Errcode& err) {
  acpt_with_fail_.CancelAccept(err);
}

void StreamAcceptorWithErrorLogic::HandleAcceptConsultErrorLogic(
  Errcode err, Stream& stm, AcceptHandler user_handler)
{
  if (err) {
    // acceptor.Close() or acceptor.Cancel() was called
    if (boost::asio::error::operation_aborted) {
      Log(_TRACE) << "Op aborted (" << err << ")\n";
      // |user_handler| is silently deleted!
      return;
    }
    bool issue_another_accept = err_logic_.OnError(err);
    if (issue_another_accept) {
      Log(_TRACE) << "Err " << err << ", issuing another accept\n";
      AsyncAcceptNofail(stm, user_handler);
    }
    else {
      Log(_TRACE) << "Err " << err << ", silently deleting handler\n";
      // |user_handler| is silently deleted!
    }
  }
  else {
    err_logic_.OnSuccess();
    user_handler();
  }
}

//  Throws if error
void StreamAcceptorBase::Open() {
  Errcode err;
  Open(err);
  // Do it exactly like boost::asio does.
  if (err) {
    throw boost::system::system_error(err);
  }
}

void StreamAcceptorBase::Bind(Endpoint addr) {
  Errcode err;
  Bind(addr, err);
  // Do it exactly like boost::asio does.
  if (err) {
    throw boost::system::system_error(err);
  }
}

void StreamAcceptorBase::StartListening() {
  Errcode err;
  StartListening(err);
  // Do it exactly like boost::asio does.
  if (err) {
    throw boost::system::system_error(err);
  }
}

net::Endpoint StreamAcceptorBase::GetLocalAddress() {
  Errcode err;
  Endpoint addr;
  GetLocalAddress(addr, err);
  if (err) {
    throw boost::system::system_error(err);
  }
  return addr;
}

net::Endpoint StreamAcceptorBase::GetLocalAddressToConnect() {
  Errcode err;
  Endpoint addr;
  GetLocalAddressToConnect(addr, err);
  if (err) {
    throw boost::system::system_error(err);
  }
  return addr;
}

void StreamAcceptorBase::CancelAccept()
{
  Errcode err;
  CancelAccept(err);
  if (err) {
    throw boost::system::system_error(err);
  }
}

}}

