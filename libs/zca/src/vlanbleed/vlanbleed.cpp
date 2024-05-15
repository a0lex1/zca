#include "vlanbleed/vlan_read_write_all.h"
#include "vlanbleed/service.h"
#include "vlanbleed/transport.h"

#include "zca/zca_common_config.h"

#include "co/async/test_kit/dbserver_stress_test.h"
#include "co/async/test_kit/dbserver_test_params.h"
#include "co/async/test_kit/test_session.h"

#include "co/async/server.h"
#include "co/async/client.h"
#include "co/async/thread_model.h"
#include "co/async/tcp.h"

#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace co;
using namespace co::async;
using namespace co::async::test_kit;
using namespace co::net;

using namespace std;
using namespace boost::asio;



// -------------------------------------------------------------------------------

// A session that executes any Session through vlan connection
class VlanSessionWrapper
  :
  public Session,
  public co::enable_shared_from_this<VlanSessionWrapper>
{
public:
  virtual ~VlanSessionWrapper() = default;

  using RefTracker = co::RefTracker;
  using RefTrackerContext = co::RefTrackerContext;
  using Stream = co::async::Stream;

  // Private CTOR, use static Shptr<> Create()
  template <typename ...Args>
  static Shptr<VlanSessionWrapper> Create(Args& ...args) {
    return Shptr<VlanSessionWrapper>(
      new VlanSessionWrapper(std::forward<Args>(args)...));
  }

  void Configure(bool accept_not_connect,
                 SessionFactoryFunc carried_sess_fac_func,
                 VlanEndpoint vlan_address/* = VlanEndpoint(1, 1)*/) {
    accept_not_connect_ = accept_not_connect;
    carried_sess_fac_func_ = carried_sess_fac_func;
    vlan_address_ = vlan_address;
  }

private:
  // Private CTOR, use static Shptr<> Create()
  VlanSessionWrapper(Uptr<Stream> s, Shptr<Strand> strand,
                     const RefTrackerContext& _dbgrtctx,
                     vlhandle_t max_accept_handles,
                     vlhandle_t max_connect_handles,
                     uint32_t queue_size,
                     uint32_t readbuf_size)
    :
    Session(move(s), strand), _dbgrtctx_(_dbgrtctx),
    max_accept_handles_(max_accept_handles), max_connect_handles_(max_connect_handles),
    queue_size_(queue_size), readbuf_size_(readbuf_size)
  {
  }

private:
  void BeginIo(RefTracker _rt) override {

    RefTracker rt_all(CUR_LOC(), [this]() {
      syslog(_DBG) << "VlWrapper " << this << " ; -= rt_all =-\n";
                      },
                      _rt);

    VlanAdapterParams adap_params;
    GetDefaultVlanAdapterParams(adap_params);

    vladap_ = make_unique<VlanAdapter>(
      GetFiberStrandShptr(),
      adap_params);

    vlsvc_ = make_unique<VlanService>(GetStream().GetIoContext(),
                                      vladap_->GetNativeApi());

    uint32_t max_chunk_body_size = co::common_config::kMaxChunkBodySize;
    vltrans_ = make_unique<VlanStreamTransport>(GetStream(),
                                                GetFiberStrandShptr(),
                                                max_chunk_body_size);
    vltrans_->SetFrameHandler(vladap_->GetFrameHandler());
    vladap_->SetFrameWriter(vltrans_->GetFrameWriter());
    vltrans_->Start(RefTracker(CUR_LOC(), [this]() {
                      syslog(_DBG) << "VlWrapper " << this << " ; rt_vltrans\n";
                      StopThreadsafe();
                    }, rt_all));
    vlstm_ = vlsvc_->CreateStreamFactory()->CreateStream();
    if (accept_not_connect_) {
      AcceptWork(RefTracker(CUR_LOC(), [this]() {
        syslog(_DBG) << "VlWrapper " << this << " ; rt_accept_work\n";
                 },rt_all));
    }
    else {
      ConnectWork(RefTracker(CUR_LOC(), [this]() {
        syslog(_DBG) << "VlWrapper " << this << " ; rt_connect_work\n";
                  }, rt_all));
    }
  }
  void StopUnsafe() override {
    // release our shared_from_this()+rt binding (drop error handler)
    vladap_->SwitchOffThreadsafe();
    Session::StopUnsafe();
  }
  void AcceptWork(RefTracker rt) {
    vlacpt_ = vlsvc_->CreateStreamAcceptorFactory()->CreateStreamAcceptor();

    VlanEndpoint vladdr(0, 666);
    vlacpt_->Open();
    vlacpt_->Bind(vladdr);
    vlacpt_->StartListening();












/////////// STRANDS BITCH!!!!!!!!!!!!!

































    vlacpt_->AsyncAccept(*vlstm_.get(), co::bind(&VlanSessionWrapper::HandleAcceptOrConnect,
                         shared_from_this(), _1, rt));
  }
  void ConnectWork(RefTracker rt) {
    vlconn_ = vlsvc_->CreateStreamConnectorFactory()->CreateStreamConnector();

    // -------------------------- MUST BE  ThreadsafeStopable SINGLE FIBER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    vlconn_->AsyncConnect(VlanEndpoint(0, 666),
                          *vlstm_.get(),
                          co::bind(&VlanSessionWrapper::HandleAcceptOrConnect,
                            shared_from_this(), _1, rt));
  }
  void HandleAcceptOrConnect(Errcode err, RefTracker rt) {
    DCHECK(!err);
    vlconn_ = nullptr;
    vlacpt_ = nullptr;
    if (!err) {
      // Virtual (carried) session will have a new strand on our Stream's io_context
      auto new_strand = make_shared<Strand>(GetStream().GetIoContext());
      Shptr<Session> carried_sess;
      carried_sess = carried_sess_fac_func_(move(vlstm_), new_strand);
      carried_sess->Start(
        RefTracker(CUR_LOC(),
        []() { 
          syslog(_DBG) << "111!!!!!11111111\n";
        }, rt));
    }
  }
  // ----------------------
  void OnVlanError(const VlanError& vlerr, RefTracker rt) {
    // Hm, VlanAdapter guarantees we're now in fiber...
    syslog(_DBG) << "VlWrapper " << this << " OnVlanError " << vlerr.MakeErrorMessage() << "\n";
    StopThreadsafe();
    // |rt| is gonna be released. Object begins to collapse. Cool, right?
  }

private:

private:
  bool accept_not_connect_{ false };
  SessionFactoryFunc carried_sess_fac_func_;
  VlanEndpoint vlan_address_;

  vlhandle_t max_accept_handles_;
  vlhandle_t max_connect_handles_;
  uint32_t queue_size_;
  uint32_t readbuf_size_;

  Uptr<Stream> vlstm_;
  Uptr<StreamConnector> vlconn_;
  Uptr<StreamAcceptor> vlacpt_;

  Uptr<VlanAdapter> vladap_;
  Uptr<VlanService> vlsvc_;
  Uptr<VlanStreamTransport> vltrans_;

  const RefTrackerContext& _dbgrtctx_;
};

// ------------------------------------------------

void test_vlan_service(TestInfo& ti) {

  static const size_t kWritePortionSize = 4096;
  static const char kWriteFillChar = 'j';
  static constexpr vlhandle_t kMaxAcceptHandles = 10;
  static constexpr vlhandle_t kMaxConnectHandles = 10;
  static constexpr uint32_t kQueueSize = 100;
  static constexpr uint32_t kReadbufSize = 60000;

  ThreadModel tm;
  io_context& iocdef(tm.DefIOC());

  Endpoint locaddr;
  auto& locaddr_vl = static_cast<VlanEndpoint&>(locaddr);

  auto s_carried_creat = [](Uptr<Stream> s, Shptr<Strand> strand) {
    auto sess = make_shared<TestSession>(move(s), strand, TestSessionParams::ReadForever());
    SET_DEBUG_TAG(*sess.get(), "s_carried_creat");
    return sess;
  };

  // we're gonna used it from here
  RefTrackerContext rtctx;

  auto s_creat = [&](Uptr<Stream> tcp_stm, Shptr<Strand> strand) {
    // locaddr already set
    Shptr<VlanSessionWrapper> vlsess;
    vlsess = VlanSessionWrapper::Create(move(tcp_stm),
                                        strand,
                                        rtctx,
                                        kMaxAcceptHandles,
                                        kMaxConnectHandles,
                                        kQueueSize,
                                        kReadbufSize);
    SET_DEBUG_TAG(*vlsess.get(), "s_creat()");
    vlsess->Configure(false/*=conn*/, s_carried_creat, locaddr_vl);
    return static_pointer_cast<Session>(vlsess);
  };
  // need c_create, c_carried_creat
  Server server(TcpEndpoint::Loopback(),
                ServerObjects(make_shared<TcpStreamFactory>(iocdef),
                              make_unique<TcpStreamAcceptor>(iocdef),
                              iocdef),
                s_creat);

  server.SetupAcceptorNowNofail();
  locaddr = server.GetLocalAddressToConnect();
  // now have locaddr

  auto testparams_client = TestSessionParams::WriteAndDisconnect(
    kWritePortionSize, kWriteFillChar);

  auto c_carried_creat = [&](Uptr<Stream> s, Shptr<Strand> strand) {
    auto sess = make_shared<TestSession>(move(s), make_shared<Strand>(iocdef),
                                         testparams_client);
    SET_DEBUG_TAG(*sess.get(), "c_carried_creat()");
    return sess;
  };
  auto c_vl_sess = VlanSessionWrapper::Create(make_unique<TcpStream>(iocdef),
                                              make_shared<Strand>(iocdef),
                                              rtctx,
                                              kMaxAcceptHandles,
                                              kMaxConnectHandles,
                                              kQueueSize,
                                              kReadbufSize);
  SET_DEBUG_TAG(*c_vl_sess.get(), "c_vl_sess");

  c_vl_sess->Configure(true/*=acpt*/, c_carried_creat, locaddr_vl);
  Client client(locaddr, make_shared<TcpStreamConnector>(), c_vl_sess);

  bool client_stopped{ false };

  RefTracker rt_all(CUR_LOC(), rtctx.GetHandle(), []() {
    syslog(_INFO) << "all stopped\n";
                    });
  server.Start(RefTracker(CUR_LOC(), [&]() {
    DCHECK(client_stopped);
    syslog(_INFO) << "server stopped (client_stopped=" << boolalpha << client_stopped << "\n";
               }, rt_all));
  client.Start(RefTracker(CUR_LOC(), [&]() {
    syslog(_INFO) << "client stopped\n";
    client_stopped = true;
    server.StopThreadsafe();
               }, rt_all));

  syslog(_INFO) << "all started, running\n";
  tm.Run();
  syslog(_INFO) << "TM returned\n";

  server.CleanupAbortedStop(); // TODO
}

// test_vlan_through_dbserver_stresstest


