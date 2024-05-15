#include "./vtester.h"

#include "co/xlog/xlog.h"

using namespace co;
using namespace co::async;
using namespace co::net;

using namespace std;
using namespace boost::asio;

VlNativeTester::VlNativeTester(io_context& iocx, uint32_t max_chunk_body_size)
  :
  iocx_(iocx), tcp_stm1_(iocx), tcp_stm2_(iocx),
  cstm_(tcp_stm1_,
        tcp_stm2_,
        make_unique<TcpStreamConnector>(),
        make_unique<TcpStreamAcceptor>(iocx), TcpEndpoint::Loopback()),
  max_chunk_body_size_(max_chunk_body_size)
{

}

void VlNativeTester::Initiate(RefTracker rt)
{
  auto on_all_stopped = [&, rt]() {
    syslog(_INFO) << " ; -= RT ALL =-\n";
    adap1_->SwitchOffThreadsafe();
  };

  RefTracker rt_all(CUR_LOC(), on_all_stopped, rt);

  cstm_.Setup([&, rt_all](Errcode e) {
    SetupVlan();
    StartVlanTransports(rt_all);
    BeginTest(rt_all); //< user's
              });
}

void VlNativeTester::HandleEmergency(VlanEmergencyContext& emctx)
{
  syslog(_ERR) << "HandleEmergency " << emctx.GetInfo().Textualize() << "\n";
}

void VlNativeTester::SetupVlan()
{
  CreateAdapterAndTransport(tcp_stm1_, adap1_, trans1_);
  CreateAdapterAndTransport(tcp_stm2_, adap2_, trans2_);
  SET_DEBUG_TAG(adap1_->DebugTag(), "AliceAdap");
  SET_DEBUG_TAG(adap2_->DebugTag(), "BobAdap");
  SET_DEBUG_TAG(trans1_->DebugTag(), "AliceTrans");
  SET_DEBUG_TAG(trans2_->DebugTag(), "BobTrans");
}

void VlNativeTester::StartVlanTransports(RefTracker rt)
{
  trans1_->Start(RefTracker(CUR_LOC(), []() {
    syslog(_INFO) << "Alice transport #ioended\n";
                 },
                 rt));
  trans2_->Start(RefTracker(CUR_LOC(), []() {
    syslog(_INFO) << "Bob transport #ioended\n";
                 },
                 rt));
}

void VlNativeTester::CreateAdapterAndTransport(Stream& stm, Uptr<VlanAdapter>& adap, Uptr<VlanStreamTransport>& trans)
{
  auto strnd = make_shared<Strand>(iocx_);

  VlanAdapterParams adap_params;
  GetDefaultVlanAdapterParams(adap_params);

  adap = make_unique<VlanAdapter>(strnd, adap_params);
  adap->SetEmergencyFn(co::bind(&VlNativeTester::HandleEmergency, this, _1));

  trans = make_unique<VlanStreamTransport>(
    stm, strnd,
    max_chunk_body_size_);

  trans->SetFrameHandler(adap->GetFrameHandler());
  adap->SetFrameWriter(trans->GetFrameWriter());
}

void StrategyTester::BeginTest(RefTracker rt)
{
  alice_strat_.SetIocCbk(GetIoContextForCallback());
  alice_strat_.SetAPI(GetAliceApi());
  alice_strat_.SetTransport(&GetAliceTransport());
  bob_strat_.SetIocCbk(GetIoContextForCallback());
  bob_strat_.SetAPI(GetBobApi());
  bob_strat_.SetTransport(&GetBobTransport());

  alice_strat_.Start(rt);
  bob_strat_.Start(rt);
}
