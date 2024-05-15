#pragma once

#include "proto/sync/sync_proto_socket.h"

#include "co/async/service.h"
#include "co/base/async_coro.h"

class ProtoCoro : public co::AsyncCoro {
public:
  virtual ~ProtoCoro() = default;

  using Service = co::async::Service;
  using Endpoint = co::net::Endpoint;

  ProtoCoro()
    :
    AsyncCoro(co::bind(&ProtoCoro::CoroEntry, this)),
    svc_(nullptr), protfac_(nullptr)
  {

  }
  ProtoCoro(Service& svc)
    : svc_(&svc), protfac_(nullptr)
  {
  }
  ProtoCoro(Service& svc, ProtoMessageFactory& protfac)
    :
    AsyncCoro(co::bind(&ProtoCoro::CoroEntry, this)),
    svc_(&svc), protfac_(&protfac)
  {
  }
  void SetLinkEndpoint(const Endpoint& ep) { link_ep_ = ep; }
  void SetService(Service& s) { svc_ = &s; }
  void SetProtoMessageFactory(ProtoMessageFactory& protfac) { protfac_ = &protfac; }

  void GetAcceptEndpoint(Endpoint& ep, Errcode& err) {
    protsock_->GetAcceptEndpoint(ep, err); 
  }

  void Initiate(Func<void()> on_ioended = Func<void()>()) {
    DCHECK(svc_);
    DCHECK(protfac_);

    on_ioended_ = on_ioended;
    protsock_ = make_unique<SyncProtoSocket>(*this, *svc_, *protfac_,
                                             link_ep_, link_ep_);

    Enter();
  }
  // Since ServerProtoTesterRunner is DTORed BEFORE ProtoCoro object, it's required
  // to call Cleanup() to destroy protsock_ before the ProtoCoro's ioc is DTORed.
  void Cleanup() {
    protsock_ = nullptr;
  }

protected:
  SyncProtoSocket& ProtoSock() { return *protsock_.get(); }

private:
  // User must use Initiate() so we are more explicit
  using AsyncCoro::Enter;

  virtual void DoInteraction() = 0;

  void CoroEntry() {
    DoInteraction();
    if (on_ioended_) {
      on_ioended_();
    }
  }
private:
  Service* svc_;
  Endpoint link_ep_;
  ProtoMessageFactory* protfac_;
  Uptr<SyncProtoSocket> protsock_;
  Func<void()> on_ioended_;
};

