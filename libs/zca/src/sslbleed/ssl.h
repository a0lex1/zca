#pragma once

#include "co/async/service.h"

// TODO: namespaces
#include "./detail/mbedtls_bio.hpp" // asio::mbedtls::
#include "./detail/mbedtls_error.hpp"
#include "./detail/mbedtls_context.hpp"
#include "./detail/mbedtls_engine.hpp"

class SslContext {
public:
  SslContext(bool client_not_server): client_not_server_(client_not_server)
  {
    if (client_not_server_) {
      context_ = make_shared<asio::ssl::mbedtls::context>(
        // what????
        asio::ssl::context_base::method::sslv3_client);
    }
    else {
      context_ = make_shared<asio::ssl::mbedtls::context>(
        asio::ssl::context_base::method::sslv3_server);
    }
  }
private:
  friend class SslStream;
  auto GetContext() { return context_; }
private:
  Shptr<asio::ssl::mbedtls::context> context_;
  bool client_not_server_;
};

// ---------------------------------------------------------------------------------------------------------------------

class SslStreamAcceptor;
class SslStreamConnector;

using HandlerWSslErr = HandlerWithErrcode;

class SslStream: public co::async::Stream {
public:
  virtual ~SslStream() = default;

  using Stream = co::async::Stream;
  using RefTracker = co::RefTracker;

  SslStream(Uptr<Stream> underl_stream);

  void Shutdown(Errcode& errcode) override {}
  void Cancel(Errcode& err) override {}
  void Close() override {}
  bool IsOpen() const override { return false; }

  void AsyncReadSome(
    boost::asio::mutable_buffers_1 buf, HandlerWithErrcodeSize handler) override;

  void AsyncWriteSome(
    boost::asio::const_buffers_1 buf, HandlerWithErrcodeSize handler) override;

  // 
  Stream& GetUnderlyingStream() { return *underl_stream_.get(); }
  Shptr<SslContext> GetSslContext() const { return ssl_ctx_; }

private:
  friend SslStreamAcceptor;
  friend SslStreamConnector;
  void SetSslContext(Shptr<SslContext> ssl_ctx) { ssl_ctx_ = ssl_ctx; }
  void Init();
  void DoHandshake(bool, HandlerWSslErr);
  void ReadHandshakeData();
  void HandleReadHandshakeData(Errcode, size_t);

private:
  Uptr<Stream> underl_stream_;
  Shptr<SslContext> ssl_ctx_;
  bool hshake_done_{ false };
  HandlerWSslErr uhandler_;
  Uptr<asio::ssl::mbedtls::engine> ssl_engine_;

  char hshake_buf_[10240];  
};

// ---------------------------------------------------------------------------------------------------------------------

class SslStreamAcceptor : public co::async::StreamAcceptor {
public:
  virtual ~SslStreamAcceptor() = default;

  using Endpoint = co::net::Endpoint;
  using Stream = co::async::Stream;
  using StreamAcceptor = co::async::StreamAcceptor;

  SslStreamAcceptor(Uptr<StreamAcceptor> underl_acpt,
                    Shptr<SslContext> sslctx)
    :
    StreamAcceptor(underl_acpt->GetIoContext()),
    underl_acpt_(std::move(underl_acpt)),
    ssl_ctx_(sslctx)
  {
  }

  // [StreamAcceptorBase impl]
  bool IsOpen() override { return underl_acpt_->IsOpen(); }
  void Open(Errcode& e) override { underl_acpt_->Open(e); }
  void Bind(Endpoint ep, Errcode& e) override { underl_acpt_->Bind(ep, e); }
  void StartListening(Errcode& e) override { underl_acpt_->StartListening(e); }
  void GetLocalAddress(Endpoint& ep, Errcode& e) override { underl_acpt_->GetLocalAddress(ep, e); }
  void GetLocalAddressToConnect(Endpoint& ep, Errcode& e) override { underl_acpt_->GetLocalAddressToConnect(ep, e); }
  void Close() override { underl_acpt_->Close(); }
  void CancelAccept(Errcode& err) override { underl_acpt_->CancelAccept(err); }

  // [StreamAcceptor impl]
  // With error code
  void AsyncAccept(
    Stream& stm,
    HandlerWithErrcode handler) override;
  void HandleUnderlAccept(Errcode err,
                          SslStream& ssl_stm,
                          HandlerWithErrcode handler);
  void HandleHandshake(Errcode err,
                       SslStream& ssl_stm,
                       HandlerWithErrcode handler);
private:
  Uptr<StreamAcceptor> underl_acpt_;
  Shptr<SslContext> ssl_ctx_;
};

class SslStreamConnector : public co::async::StreamConnector {
public:
  virtual ~SslStreamConnector() = default;

  using StreamConnector = co::async::StreamConnector;
  using Endpoint = co::net::Endpoint;
  using Stream = co::async::Stream;

  SslStreamConnector(StreamConnector& underlying_connector, Shptr<SslContext> sslctx);

  void AsyncConnect(Endpoint addr, Stream& stm, HandlerWithErrcode handler) override;
};

// ---------------------------------------------------------------------------------------------------------------------

class SslStreamFactory : public co::async::StreamFactory {
public:
  virtual ~SslStreamFactory() = default;

  using StreamFactory = co::async::StreamFactory;
  using Stream = co::async::Stream;

  SslStreamFactory(Uptr<StreamFactory> underl_fac)
    :
    StreamFactory(underl_fac->GetIoContext()),
    underl_fac_(std::move(underl_fac))
  {

  }

  Uptr<Stream> CreateStream() override {
    auto s = underl_fac_->CreateStream();
    return make_unique<SslStream>(move(s));
  }

private:
  Uptr<StreamFactory> underl_fac_;
};

// ---------------------------------------------------------------------------------------------------------------------

class SslStreamAcceptorFactory : public co::async::StreamAcceptorFactory {
public:
  virtual ~SslStreamAcceptorFactory() = default;

  using StreamAcceptor = co::async::StreamAcceptor;
  using StreamAcceptorFactory = co::async::StreamAcceptorFactory;

  SslStreamAcceptorFactory(Uptr<StreamAcceptorFactory> underl_fac,
                           Shptr<SslContext> ssl_ctx)
    : underl_fac_(std::move(underl_fac)), ssl_ctx_(ssl_ctx)
  {
  }
  Uptr<StreamAcceptor> CreateStreamAcceptor() override {
    return make_unique<SslStreamAcceptor>(
      underl_fac_->CreateStreamAcceptor(),
      ssl_ctx_);
  }
private:
  Uptr<StreamAcceptorFactory> underl_fac_;
  Shptr<SslContext> ssl_ctx_;
};

class SslStreamConnectorFactory : public co::async::StreamConnectorFactory {
public:
  virtual ~SslStreamConnectorFactory() = default;

  using StreamConnector = co::async::StreamConnector;


};

// ---------------------------------------------------------------------------------------------------------------------

// not used yet?
class SslService : public co::async::Service {
public:
  virtual ~SslService() = default;

  using Service = co::async::Service;
  using StreamFactory = co::async::StreamFactory;
  using StreamAcceptorFactory = co::async::StreamAcceptorFactory;
  using StreamConnectorFactory = co::async::StreamConnectorFactory;

  SslService(Service& underl_svc) : underl_svc_(underl_svc)
  {
  }

  void Configure(Shptr<SslContext> ssl_acpt_ctx,
                 Shptr<SslContext> ssl_conn_ctx) {
    ssl_acpt_ctx_ = ssl_acpt_ctx;
    ssl_conn_ctx_ = ssl_conn_ctx;
  }
  Uptr<StreamFactory> CreateStreamFactory() override {
    return make_unique<SslStreamFactory>(
      underl_svc_.CreateStreamFactory());
  }
  Uptr<StreamAcceptorFactory> CreateStreamAcceptorFactory() override {
    return make_unique<SslStreamAcceptorFactory>(
      underl_svc_.CreateStreamAcceptorFactory(),
      ssl_acpt_ctx_);
  }
  Uptr<StreamConnectorFactory> CreateStreamConnectorFactory() override {
    //return make_unique<SslStreamConnectorFactory>(underl_svc_.GetStreamFactory());
    return nullptr;
  }
private:
  Service& underl_svc_;
  Shptr<SslContext> ssl_acpt_ctx_;
  Shptr<SslContext> ssl_conn_ctx_;
};

