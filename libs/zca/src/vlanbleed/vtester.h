#pragma once

#include "./connected_streams2.h"
#include "./transport.h"
#include "./adapter.h"
#include "./vlan_read_write_all.h"

// non-stopable
class VlNativeTester {
public:
  virtual ~VlNativeTester() = default;

  using RefTracker = co::RefTracker;
  using Stream = co::async::Stream;
  using TcpStream = co::async::TcpStream;

  VlNativeTester(io_context& iocx, uint32_t max_chunk_body_size);

  void Initiate(RefTracker rt);

private:
  virtual void BeginTest(RefTracker rt) = 0;

protected:
  VlanNativeApi& GetAliceApi() { return adap1_->GetNativeApi(); }
  VlanNativeApi& GetBobApi() { return adap2_->GetNativeApi(); }
  VlanTransport& GetAliceTransport() { return *trans1_.get(); }
  VlanTransport& GetBobTransport() { return *trans2_.get(); }
  io_context& GetIoContextForCallback() { return iocx_; }

private:
  void HandleEmergency(VlanEmergencyContext& emctx);

  void SetupVlan();
  void StartVlanTransports(RefTracker rt);
  void CreateAdapterAndTransport(Stream& stm,
                                 Uptr<VlanAdapter>& adap,
                                 Uptr<VlanStreamTransport>& trans);
private:
  uint32_t max_chunk_body_size_{ 0 };
  io_context& iocx_;
  TcpStream tcp_stm1_;
  TcpStream tcp_stm2_;
  ConnectedStreams2 cstm_;
  Uptr<VlanAdapter> adap1_;
  Uptr<VlanAdapter> adap2_;
  Uptr<VlanStreamTransport> trans1_;
  Uptr<VlanStreamTransport> trans2_;
};

// ----------------------------------------------------------------------------------------------------------------

class VTesterStrategy : public co::async::Startable {
public:
  ritual ~VTesterStrategy() = default;

protected:
  VlanNativeApi& API() { return *vlapi_; }
  io_context& IocForCbk() { return *ioc_cbk_; }
  void StopTransport() {
    vltrans_->StopThreadsafe();
  }
private:
  friend class StrategyTester;
  void SetIocCbk(io_context& ioc_cbk) { ioc_cbk_ = &ioc_cbk; }
  void SetAPI(VlanNativeApi& api) { vlapi_ = &api; }
  void SetTransport(VlanTransport* t) { vltrans_ = t; }
private:
  io_context* ioc_cbk_{ nullptr };
  VlanNativeApi* vlapi_{ nullptr };
  VlanTransport* vltrans_;
};

// ----------------------------------------------------------------------------------------------------------------

class StrategyTester : public VlNativeTester {
public:
  StrategyTester(io_context& ioc,
          VTesterStrategy& alice_strat, VTesterStrategy& bob_strat,
          uint32_t max_chunk_body_size)
    :
    VlNativeTester(ioc, max_chunk_body_size),
    alice_strat_(alice_strat), bob_strat_(bob_strat)
  {
  }

private:
  void BeginTest(RefTracker rt) override;
private:
  VTesterStrategy& alice_strat_;
  VTesterStrategy& bob_strat_;
};


