#pragma once

#include "proto/proto_message_writer.h"
#include "proto/proto_message_reader.h"

#include "co/async/service.h"
#include "co/base/async_coro.h"
#include "co/common_config.h"

class SyncProtoSocket {
public:
  using AsyncCoro = co::AsyncCoro;
  using Service = co::async::Service;
  using Endpoint = co::net::Endpoint;
  using StreamConnector = co::async::StreamConnector;
  using StreamAcceptor = co::async::StreamAcceptor;
  using StreamChunkReader = co::async::StreamChunkReader;
  using StreamChunkWriter = co::async::StreamChunkWriter;
  using Stream = co::async::Stream;

  SyncProtoSocket(AsyncCoro& host_coro,
                  Service& service,
                  ProtoMessageFactory& protfac,
                  const Endpoint& default_address_accept,
                  const Endpoint& default_address_connect,
                  uint32_t max_chunk_body_size = co::common_config::kMaxChunkBodySize)
    :
    host_coro_(host_coro), service_(service), protfac_(protfac),
    def_addr_acpt_(default_address_accept),
    def_addr_conn_(default_address_connect),
    max_chunk_body_size_(max_chunk_body_size)
  {

  }

  Errcode ConnectServer() {
    return _ConnectServer(nullptr);
  }
  Errcode ConnectServer(const Endpoint& remaddr) {
    return _ConnectServer(&remaddr);
  }
  Errcode AcceptClient() {
    return _AcceptClient(nullptr);
  }
  Errcode AcceptClient(const Endpoint& locaddr) {
    return _AcceptClient(&locaddr);
  }
  Errcode WriteProtoMessage(const ProtoMessage& pmsg) {
    DCHECK(connected_);
    auto cbk(host_coro_.CreateContinuationCallback<Errcode> ());
    host_coro_.DoYield([&]() { // *YIELD POINT*
      protwrit_->AsyncWriteMessage(pmsg, cbk.GetFunction());
                      });
    // *UNYIELD* i/o returned
    return std::get<0>(cbk.ResultTuple());
  }
  ProtoError ReadProtoMessage(Uptr<ProtoMessage>& pmsg) {
    DCHECK(connected_);
    auto cbk(host_coro_.CreateContinuationCallback<ProtoError>());
    host_coro_.DoYield([&]() { // *YIELD POINT*
      protrdr_->AsyncReadMessage(pmsg, cbk.GetFunction());
                      });
    // *UNYIELD* i/o returned
    return std::get<0>(cbk.ResultTuple());
  }
  Errcode Shutdown() {
    Errcode err;
    stm_->Shutdown(err);
    return err;
  }
  void Close() {
    stm_->Close();
  }

  void GetAcceptEndpoint(Endpoint& ep, Errcode& err) {
    stm_acpt_->GetLocalAddressToConnect(ep, err);
  }

private:
  void OnConnectionEstablished() {
    DCHECK(!connected_);
    DCHECK(protwrit_ == nullptr);
    DCHECK(protrdr_ == nullptr);

    ioc_svc_ = &service_.CreateStreamFactory()->CreateStream()->GetIoContext();

    // Delete acceptor/connector, no need to keep it
    stm_acpt_ = nullptr;
    stm_conn_ = nullptr;
    // Create objects.
    // We don't need ST queue writer. We read and write in one [async coro's] fiber.
    // #StrandOr
    auto strand = make_shared<Strand>(*ioc_svc_);
    chunkwrit_ = make_unique<StreamChunkWriter>(strand, *stm_.get());
    protwrit_ = make_unique<ProtoMessageWriter>(*chunkwrit_.get());
    chunkrdr_ = make_unique<StreamChunkReader>(strand, *stm_.get(), max_chunk_body_size_);
    protrdr_ = make_unique<ProtoMessageReader>(*chunkrdr_.get(), protfac_);
    // Tag that we are connected. Can now read/write proto messages.
    connected_ = true;
  }

  Errcode _AcceptClient(const Endpoint* locaddr) {
    DCHECK(!connected_);
    Endpoint ep_use;
    if (locaddr != nullptr) {
      ep_use = *locaddr;
    }
    else {
      ep_use = def_addr_acpt_;
    }
    stm_ = service_.CreateStreamFactory()->CreateStream();
    stm_acpt_ = service_.CreateStreamAcceptorFactory()->CreateStreamAcceptor();
    Errcode err;
    stm_acpt_->Open(err);
    if (!err) {
      stm_acpt_->Bind(ep_use, err);
      if (!err) {
        stm_acpt_->StartListening(err);
        if (!err) {
          err = SyncAccept(*stm_acpt_.get(), *stm_.get());

          if (!err) {
            OnConnectionEstablished();
          }
        }
      }
    }
    return err;
  }

  Errcode _ConnectServer(const Endpoint* remaddr) {
    DCHECK(!connected_);
    Endpoint ep_use;
    if (remaddr != nullptr) {
      ep_use = *remaddr;
    }
    else {
      ep_use = def_addr_conn_;
    }
    stm_ = service_.CreateStreamFactory()->CreateStream();
    stm_conn_ = service_.CreateStreamConnectorFactory()->CreateStreamConnector();
    Errcode err =
      SyncConnect(*stm_conn_.get(), ep_use, *stm_.get());
    if (!err) {
      OnConnectionEstablished();
    }
    return err;
  }

  Errcode SyncConnect(StreamConnector& conn, const Endpoint& ep, Stream& stm) {
    DCHECK(!connected_);
    auto cbk(host_coro_.CreateContinuationCallback<Errcode>());
    host_coro_.DoYield([&, ep]() { // *YIELD POINT*
      conn.AsyncConnect(ep, stm, cbk.GetFunction());
                      });
    // *UNYIELD* i/o returned
    return std::get<0>(cbk.ResultTuple());
  }

  Errcode SyncAccept(StreamAcceptor& acpt, Stream& stm) {
    DCHECK(!connected_);
    auto cbk(host_coro_.CreateContinuationCallback<Errcode>());
    host_coro_.DoYield([&]() { // *YIELD POINT*
      acpt.AsyncAccept(stm, cbk.GetFunction());
                      });
    // *UNYIELD* i/o returned
    return std::get<0>(cbk.ResultTuple());
  }

private:
  AsyncCoro& host_coro_;
  Service& service_;
  ProtoMessageFactory& protfac_;
  Endpoint def_addr_acpt_;
  Endpoint def_addr_conn_;
  uint32_t max_chunk_body_size_;

  Uptr<Stream> stm_;
  Uptr<StreamAcceptor> stm_acpt_;
  Uptr<StreamConnector> stm_conn_;
  bool connected_{ false };

  Uptr<StreamChunkWriter> chunkwrit_;
  Uptr<StreamChunkReader> chunkrdr_;
  Uptr<ProtoMessageWriter> protwrit_;
  Uptr<ProtoMessageReader> protrdr_;

  io_context* ioc_svc_{nullptr};
};


