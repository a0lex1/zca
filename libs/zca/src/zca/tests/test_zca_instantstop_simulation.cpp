// No zca dependencies, only co.

#include "co/async/test_kit/test_session.h"
#include "co/async/configs/thread_model_config_from_dict.h"

#include "co/async/wrap_post.h"
#include "co/async/tcp_service.h"
#include "co/async/tcp_stream_connector.h"
#include "co/async/thread_model.h"
#include "co/async/threadsafe_stopable_impl.h"
#include "co/async/server.h"
#include "co/async/client.h"

#include "co/async/sync/sync_event.h"

#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::net;
using namespace co::async;
using namespace co::async::configs;
using namespace co::async::test_kit;
using namespace boost::asio;

#define llog() syslog(_TRACE) << "test_zca_inststop_sim "

namespace {

static const uint16_t kDefaultXxxPort = 54728;

// Simulates Backend
class Pornhub : public co::async::Startable, public co::async::Stopable, public co::async::ThreadsafeStopable {
public:
  virtual ~Pornhub() = default;

  Pornhub(io_context& ioc, uint16_t serv_port = kDefaultXxxPort)
    : ioc_(ioc), tss_impl_(*this, make_shared<Strand>(ioc))
  {
    addr_ = TcpEndpoint("127.0.0.1", serv_port);
    srv_ = make_unique<ServerWithSessList>(
          addr_,
          make_shared<TcpService>(ioc_),
          [](Uptr<Stream> stm, Shptr<Strand> strand) {
     return make_shared<TestSession>(move(stm), strand, TestSessionParams::ReadForever());
               });
  }
  void PrepareToStartNofail() {
    llog() << "Pornhub : srv_.PrepareToStartNofail()... \n";
    srv_->PrepareToStartNofail();
  }
  void Start(RefTracker rt) override {
    tss_impl_.BeforeStartWithIoEnded(rt, rt);

    RefTracker rt_all(CUR_LOC(),
                      [&] () {
                 llog() << "Pornhub : srv_ #ioended\n";
                      },
                      rt);

    llog() << "Pornhub : starting srv_ ...\n";
    srv_->Start(rt_all);
  }
  void StopThreadsafe() override {
    llog() << "Pornhub : StopThreadsafe\n";
    tss_impl_.StopThreadsafe();
  }

private:
  void StopUnsafe() override {
    if (srv_) {
      llog() << "Pornhub : StopUnsafe - stopping srv_\n";
      // Allow StopUnsafe() before Start(). Is it bad?
      srv_->StopThreadsafe();
    }
    else {
      llog() << "Pornhub : srv_ == nullptr. Not started yet\n";
    }
  }

private:
  io_context& ioc_;
  ThreadsafeStopableImpl tss_impl_;
  Uptr<ServerWithSessList> srv_; // simulates cc server
  Endpoint addr_;
};

// Simulates Backend
class Pornhub2 : public co::async::Startable, public co::async::ThreadsafeStopable {
public:
  virtual ~Pornhub2() = default;

  Pornhub2(io_context& ioc, uint16_t serv_port = kDefaultXxxPort)
    : ioc_(ioc)
  {
    addr_ = TcpEndpoint("127.0.0.1", serv_port);
    srv_ = make_unique<ServerWithSessList>(
          addr_,
          make_shared<TcpService>(ioc_),
          [](Uptr<Stream> stm, Shptr<Strand> strand) {
     return make_shared<TestSession>(move(stm), strand, TestSessionParams::ReadForever());
               });
  }
  void SetupAcceptorNowNofail() {
    llog() << "Pornhub2 : srv_.SetupAcceptorNowfail()... \n";
    srv_->SetupAcceptorNowNofail();
  }
  void Start(RefTracker rt) override {
    RefTracker rt_all(CUR_LOC(),
                      [&] () {
                 llog() << "Pornhub2 : srv_ #ioended\n";
                      },
                      rt);

    llog() << "Pornhub2 : starting srv_ ...\n";
    srv_->Start(rt_all);
  }
  void StopThreadsafe() override {
    llog() << "Pornhub2 : StopThreadsafe\n";
    srv_->StopThreadsafe();
  }

private:
  io_context& ioc_;
  Uptr<ServerWithSessList> srv_; // simulates cc server
  Endpoint addr_;
};

class JerkerSession : public Session, public co::enable_shared_from_this<JerkerSession> {
public:
  virtual ~JerkerSession() = default;

  using Session::Session;

private:
  void BeginIo(RefTracker rt) override {
    llog() << "BeginIo, reading some\n";

    rt.SetReferencedObject(shared_from_this());

    RefTracker rt_all(CUR_LOC(),
                      [] () {
      llog() << "; rt_all\n";
                    },
                    rt);
    BeginIoInternal(rt_all);
  }
  void BeginIoInternal(RefTracker rt) {
    GetStream().AsyncReadSome(
          mutable_buffers_1(read_buf_, 1),
          wrap_post(GetFiberStrand(), co::bind(&JerkerSession::HandleReadSome, shared_from_this(), _1, _2, rt)));
  }
  void HandleReadSome(Errcode err, size_t num_bytes, RefTracker rt) {
    // Peer is not supposed to send any data
    DCHECK(num_bytes == 0);
    llog() << "HandleReadSome, err = " << err << "\n";
  }
private:
  char read_buf_[1];
};

// Simulates Agent
class Jerker : public co::async::Startable, public co::async::ThreadsafeStopable {
public:
  virtual ~Jerker() = default;

  Jerker(io_context& ioc, uint16_t serv_port = kDefaultXxxPort)
    : ioc_(ioc)
  {
    addr_ = TcpEndpoint("127.0.0.1", serv_port);
  }
  void Start(RefTracker rt) override {
    llog() << "Jerker : starting cli_ ...\n";

    auto stm = make_unique<TcpStream>(ioc_);
    auto strand = make_shared<Strand>(ioc_);
    cli_ = make_unique<Client>(addr_,
                               make_shared<TcpStreamConnector>(),
                               make_shared<JerkerSession>(move(stm), strand));

    RefTracker rt_all(CUR_LOC(),
                      [&] () {
      llog() << "Jerker : cli_ #ioended, connect error = " << cli_->GetConnectError() << "\n";
                      },
                      rt);
    cli_->PrepareToStartNofail();
    cli_->Start(rt_all);
  }
  void StopThreadsafe() override {
    llog() << "Pornhub : StopThreadsafe\n";
    cli_->StopThreadsafe();
  }

private:
  io_context& ioc_;
  Uptr<Client> cli_;
  Endpoint addr_;
};
} // namespace

void test_zca_instantstop_simulation(TestInfo& ti) {

  ThreadModelConfigFromDict tmconf(ThreadModelConfig(), ti.opts_dict, ConsumeAction::kDontConsume);
  ThreadModel tm(tmconf);

  Pornhub ph(tm.DefIOC(), 1337);
  Jerker jr(tm.DefIOC(), 1337);

  RefTrackerContext rtctx(CUR_LOC());

  ph.PrepareToStartNofail();

  RefTracker rt_all(CUR_LOC(), rtctx.GetHandle(),
                    [] () {
    llog() << "; rt_all\n";
  });
  ph.Start(RefTracker(CUR_LOC(),
           [] () {
          llog() << "* Pornhub #ioended\n";
            },
            rt_all));
  jr.Start(RefTracker(CUR_LOC(),
           [] () {
          llog() << "* Jerker #ioended\n";
            },
            rt_all));
  rt_all = RefTracker();
  ph.StopThreadsafe();
  jr.StopThreadsafe();

  llog() << "* doing tm.Run()\n";
  tm.Run();
  llog() << "* tm.Run() returned\n";

  rtctx._DbgLogPrintTrackedRefTrackers();
  DCHECK(0 == rtctx.GetAtomicRefTrackerCount());

  //log() << "ct stopped, returning\n";
}



