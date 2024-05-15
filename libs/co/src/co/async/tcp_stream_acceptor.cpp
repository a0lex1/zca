#include "co/async/tcp_stream_acceptor.h"
#include "co/async/tcp_stream.h"
#include "co/xlog/xlog.h"

using namespace std;
using co::net::TcpEndpoint;

#define llog() Log(_TRACE) << "TcpStmAcpt " << SPTR(this) << " "

namespace co {
namespace async {

DEFINE_XLOGGER_SINK("tcpacceptor", gCoAsyncTcpStreamAcceptorLogSink); // defined in stream_acceptor.h
#define XLOG_CURRENT_SINK gCoAsyncTcpStreamAcceptorLogSink

TcpStreamAcceptor::TcpStreamAcceptor(io_context& ioc) :
  StreamAcceptor(ioc), acpt_(ioc)
{

}

void TcpStreamAcceptor::Close() {
  acpt_.close();
}

void TcpStreamAcceptor::AsyncAccept(
    Stream& str, HandlerWithErrcode handler)
{
  acpt_.async_accept(static_cast<TcpStream&>(str).GetSocket(),
                     [/*&,*/ handler](Errcode err) {
                       // INSIDE UNKNOWN FIBER
                       //Log(_DBG) << "TCP STM ACPT : async_accept err=" << err << "\n";
                       handler(err);
                       //(this);
                     });
}

bool TcpStreamAcceptor::IsOpen() {
  return acpt_.is_open();
}

void TcpStreamAcceptor::Open(Errcode& err) {
  // eventually add IPv6 support
  acpt_.open(boost::asio::ip::tcp::v4(), err);
}

void TcpStreamAcceptor::Bind(Endpoint addr, Errcode& err) {
#ifdef CO_ACPT_TCP_REUSEADDR
  boost::asio::socket_base::reuse_address option(true);
  acpt_.set_option(option, err);
#endif
  TcpEndpoint tcp_addr(addr);
  if (!err) {
    llog() << "Binding to " << addr.ToString() << " ...\n";
    acpt_.bind(tcp_addr.GetAddr(), err);
    if (!err) {
      llog() << "Bound to " << addr.ToString() << ".\n";
      return;
    }
    else {
      llog() << "Can't bind to " << addr.ToString() << " error " << err << "\n";
    }
  }
  acpt_.close();
  // BUGBUG1 test_text_session()'s i==3703 Bind() generates 10013 (WSAEACCES); reuse_addr on/off doesn't change anything
}

void TcpStreamAcceptor::StartListening(Errcode& err) {
  acpt_.listen(boost::asio::socket_base::max_listen_connections, err);
}

void TcpStreamAcceptor::GetLocalAddress(Endpoint& addr, Errcode& err) {
  addr = TcpEndpoint(acpt_.local_endpoint(err));
}

void TcpStreamAcceptor::GetLocalAddressToConnect(Endpoint& addr, Errcode& err) {
  TcpEndpoint real_addr;
  GetLocalAddress(real_addr, err);
  if (err) {
    return;
  }
  if (real_addr.GetAddr().address().is_unspecified()) {
    // if bound to 0.0.0.0, replace to 127.0.0.1
    addr = TcpEndpoint::Loopback(real_addr.GetAddr().port());
  }
  else {
    addr = real_addr;
  }
}

void TcpStreamAcceptor::CancelAccept(Errcode& err) {
  acpt_.cancel(err);
  //Log(_DBG) << "TCP STM ACPT : cancel() returned err=" << err << "\n";
}

}
}
