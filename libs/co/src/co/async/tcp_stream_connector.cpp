#include "co/async/tcp_stream_connector.h"
#include "co/xlog/xlog.h"

using namespace std;

namespace co {
namespace async {

DEFINE_XLOGGER_SINK("connector", gCoAsyncStreamConnectorLogSink);
#define XLOG_CURRENT_SINK gCoAsyncStreamConnectorLogSink


void TcpStreamConnector::AsyncConnect(Endpoint addr,
                                      Stream& stm,
                                      HandlerWithErrcode handler) {
  // (check without dynamic_cast)
  TcpStream& tcp_stm(static_cast<TcpStream&>(stm));
  net::TcpEndpoint tcp_addr(addr);
  tcp_stm.GetSocket().async_connect(tcp_addr.GetAddr(),
                                    [=](Errcode err) {
                                      handler(err);
                                    });
}

}}


