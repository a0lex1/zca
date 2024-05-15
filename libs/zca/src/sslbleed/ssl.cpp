#include "./ssl.h"

#include "co/base/debug_tag_owner.h"
#include "co/xlog/xlog.h"

using namespace std;

SslStream::SslStream(Uptr<Stream> underl_stream) :
  Stream(underl_stream->GetIoContext()),
  underl_stream_(std::move(underl_stream))
{

}

void SslStream::AsyncReadSome(boost::asio::mutable_buffers_1 buf, HandlerWithErrcodeSize handler)
{
  //<>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<
}

void SslStream::AsyncWriteSome(boost::asio::const_buffers_1 buf,
                               HandlerWithErrcodeSize handler)
{

}

void SslStream::Init()
{
  DCHECK(ssl_ctx_);
  DCHECK(underl_stream_);
  ssl_engine_ = make_unique<asio::ssl::mbedtls::engine>(ssl_ctx_->GetContext()); //we're friends
}

namespace {
void LogMbedBio(asio::ssl::mbedtls::bio* extbio, const string& tag) {
  syslog(_DBG) << " [mbed-bio-" << tag << "] "
    << "wpending=" << extbio->wpending()
    << ", ctrl_pending=" << extbio->ctrl_pending()
    << ", should_write=" << boolalpha << extbio->should_write()
    << ", should_read=" << boolalpha << extbio->should_read() << "\n";
}
}

void SslStream::DoHandshake(bool client_not_server, HandlerWSslErr handler)
{
  DCHECK(!hshake_done_);
  int r;
  uhandler_ = handler;
  hshake_done_ = false;
  if (client_not_server) {
    // cline role
    LogMbedBio(ssl_engine_->ext_bio(), "client");
    r = ssl_engine_->connect();
    syslog(_DBG) << "ssl_engine_->connect() = " << r << "\n";
    LogMbedBio(ssl_engine_->ext_bio(), "client");
  }
  else {
    // server role
    r = ssl_engine_->accept();
    syslog(_DBG) << "ssl_engine_->accept() = " << r << "\n";
    LogMbedBio(ssl_engine_->ext_bio(), "server");

    DCHECK(r == MBEDTLS_ERR_SSL_WANT_READ);

    ReadHandshakeData();
  }
}

void SslStream::ReadHandshakeData() {
  DCHECK(!hshake_done_);
  AsyncReadSome(boost::asio::mutable_buffers_1(hshake_buf_, sizeof(hshake_buf_)),
                co::bind(&SslStream::HandleReadHandshakeData, this, _1, _2));
}

void SslStream::HandleReadHandshakeData(Errcode e, size_t num_bytes) {
  DCHECK(!hshake_done_);
  int r;
  if (!e) {
    ssl_engine_->ext_bio()->write(hshake_buf_, num_bytes);
    r = ssl_engine_->accept();

    char buf[1024];
    r = ssl_engine_->ext_bio()->read(buf, 1024);
  }
}

// --------------------------------------------------------------------------------------------------------

void SslStreamAcceptor::AsyncAccept(Stream& stm, HandlerWithErrcode handler)
{
  SslStream& ssl_stm(static_cast<SslStream&>(stm));
  underl_acpt_->AsyncAccept(ssl_stm.GetUnderlyingStream(),
                            co::bind(&SslStreamAcceptor::HandleUnderlAccept,
                            this, _1, std::ref(ssl_stm), handler));
}

void SslStreamAcceptor::HandleUnderlAccept(Errcode err, SslStream& ssl_stm,
                                           HandlerWithErrcode handler)
{
  if (!err) {
    // we're friends relax
    ssl_stm.SetSslContext(ssl_ctx_);
    ssl_stm.Init();
    ssl_stm.DoHandshake(
      false/*server*/,
      co::bind(&SslStreamAcceptor::HandleHandshake,
      this, _1, std::ref(ssl_stm), handler));
  }
}

void SslStreamAcceptor::HandleHandshake(Errcode err, SslStream& ssl_stm,
                                        HandlerWithErrcode handler)
{
  // err->ssl_error
  if (!err) {
    // OK, the Stream is ready to read/write 
    handler(co::NoError());
  }
  else {
    handler(err);
  }
}

