#include "co/async/tcp_stream.h"
#include "co/xlog/xlog.h"
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>

using namespace std;
namespace co {

namespace async {

tcp_socket& TcpStream::GetSocket()
{
  return tcp_sock_;
}

void TcpStream::AsyncReadSome(boost::asio::mutable_buffers_1 buf, HandlerWithErrcodeSize handler)
{
  tcp_sock_.async_read_some(buf, handler);
}

void TcpStream::AsyncWriteSome(boost::asio::const_buffers_1 buf, HandlerWithErrcodeSize handler)
{
  tcp_sock_.async_write_some(buf, handler);
}

void TcpStream::Shutdown(Errcode& err)
{
  tcp_sock_.shutdown(boost::asio::socket_base::shutdown_both, err);
}

void TcpStream::Cancel(Errcode& err)
{
  tcp_sock_.cancel(err);
}

void TcpStream::Close()
{
  tcp_sock_.close();
}

bool TcpStream::IsOpen() const
{
  return tcp_sock_.is_open();
}

void TcpStream::GetLocalAddress(co::net::Endpoint& addr, Errcode& err) {
  boost::asio::ip::tcp::endpoint ep = tcp_sock_.local_endpoint(err);
  if (err) {
    return;
  }
  addr = co::net::TcpEndpoint(ep);
}

void TcpStream::GetRemoteAddress(co::net::Endpoint& addr, Errcode& err) {
  boost::asio::ip::tcp::endpoint ep = tcp_sock_.remote_endpoint(err);
  if (err) {
    return;
  }
  addr = co::net::TcpEndpoint(ep);
}


}}


