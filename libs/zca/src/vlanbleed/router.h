#pragma once

namespace route {

class RoutingAcceptor : public StreamAcceptor {
public:
  virtual ~RoutingAcceptor() = default;
};

class LocalTable {
public:
  std::map<VlanAddress, StreamIoService*>& Get() {
    return table_;
  }
private:
  std::map<VlanAddress, StreamIoService*> table_;
};

/*************************************************
* 
* 
* ******************
* 
* 
* todo: GENERIC PATTERN (for abstract StreamIoService-s, not just for VLAN)
* 
* ******************
* 
* 
* ************************************************
*/

class RoutingConnector {

  private:
  struct Req {
    Stream& stm;
    Endpoint endpoint;
    HandlerWithErrcode uhandler;
  };

public:
  virtual ~RoutingConnector() = default;

  void AsyncConnect(Stream&, Endpoint addr, HandlerWithErrcode handler) override {
    // inside unknown fiber
    auto& vladdr(static_cast<VlanEndpoint&>(addr));
    auto vlstm(static_cast<VlanStream&>(stm));

    // -------------------------- MUST BE  ThreadsafeStopable SINGLE FIBER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    Shptr<Req> req{ stm, endpoint, handler };
    FindLocalAddressThreadsafe(
      addr, co::bind(&RoutingConnector::HandleFindAddress, this,
      req));
  }
private:
  void HandleFindAddress(Errcode err, StreamIoService& service, Shptr<Req> req) {
    // inside bot list fiber
    if (it_found != bl.end()) {
      Shptr<ICcBot> bot = *it_found;
      vlan_cnn_ = bot->GetVlanService().GetConnectorFactory().CreateConnector();

      vlan_cnn_->AsyncConnect(req->stm, req->endpoint, req->uhandler);
    }
  }
  friend class VlanRouter;
  RoutingConnector(LocalTable& local_tbl) : local_tbl_(local_tbl){
  }
  void FindLocalAddressThreadsafe(VlanAddress addr, );
private:
  LocalTable& local_tbl_;
  Uptr<StreamConnector> vlan_cnn_;
};

class VlanRouter : public StreamIoService, public StreamFactory {
public:
  //virtual ~;

  void AddAdapter(StreamIoService& adapter_service, VlanAddress local_address) {

  }

  StreamFactory& GetStreamFactory() { return *this; }
  //StreamAcceptorFactory&

  Uptr<StreamAcceptor> CreateAcceptor() { return make_unique<RoutingAcceptor>(); };
  Uptr<StreamConnector> CreateConnector() { return make_unique<RoutingConnector>(); };

private:
  Uptr<Stream> CreateStream() override {
    return make_unique<>();
  }
private:
   local_table_;
};











































