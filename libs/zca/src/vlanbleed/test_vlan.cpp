#include "./vtester.h"

#include "./vlan_read_write_all.h"
#include "./service.h"
#include "./transport.h"
#include "./vlan_common.h"

#include "co/base/tests.h"
#include "co/common_config.h"

#include "co/xlog/xlog.h"

using namespace co;
using namespace co::async;

using namespace std;
using namespace boost::asio;

namespace {
class MyAliceStrat : public VTesterStrategy {
public:
  virtual ~MyAliceStrat() = default;

private:
  void Start(RefTracker rt) override {
    // -------------------------- MUST BE  ThreadsafeStopable SINGLE FIBER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    API().VnAccept(hAcpt_,
                   VlanEndpoint(1, 1),
                   co::bind(&MyAliceStrat::HandleAccept, this, _1, rt),
                   IocForCbk());
  }
  void HandleAccept(VlanError vlerr, RefTracker rt) {
    DCHECK(!vlerr);
    syslog(_INFO) << "Alice accepted Bob, err => " << vlerr.MakeErrorMessage() << "\n";
    // ok
    VlanReadAll(API(), hAcpt_, mutable_buffers_1(vnreadbuf_, sizeof(vnreadbuf_)),
                co::bind(&MyAliceStrat::HandleReadAll, this, _1, _2, rt),
                IocForCbk());
  }
  void HandleReadAll(VlanError vlerr, size_t bytes_read, RefTracker rt) {
    DCHECK(!vlerr);
    syslog(_INFO) << "Alice has read " << bytes_read << " bytes from bob, err " << vlerr.MakeErrorMessage() << "\n";
    if (!vlerr) {
      syslog(_INFO) << "Alice: shutdown+close...\n";

      VlanError vlerr;
      API().VnShutdownSend(hAcpt_, vlerr);
      DCHECK(!vlerr);

      API().VnClose(hAcpt_);
    }
  }
private:
  vlhandle_t hAcpt_{ 0 };
  char vnreadbuf_[1024];
};

class MyBobStrat : public VTesterStrategy {
public:
  virtual ~MyBobStrat() = default;

private:
  void Start(RefTracker rt) override {
    API().VnConnect(hConn_, VlanEndpoint(1, 1),
                    co::bind(&MyBobStrat::HandleConnect, this, _1, rt),
                    IocForCbk());
  }
  void HandleConnect(VlanError vlerr, RefTracker rt) {
    DCHECK(!vlerr);
    syslog(_INFO) << "Bob connected, err => " << vlerr.MakeErrorMessage() << "\n";
    // ok
    syslog(_INFO) << "Bob writing to Alice...\n";
    memset(vnwritebuf_, 'y', sizeof(vnwritebuf_));
    VlanWriteAll(API(), hConn_, const_buffers_1(vnwritebuf_, sizeof(vnwritebuf_)),
                 co::bind(&MyBobStrat::HandleWriteAll, this, _1, _2, rt),
                 IocForCbk());
  }
  void HandleWriteAll(VlanError vlerr, size_t num_bytes, RefTracker rt) {
    DCHECK(!vlerr);
    syslog(_INFO) << "Bob has written [all " << num_bytes << " b] data, err = > " << vlerr.MakeErrorMessage() << "\n";

    API().VnRead(hConn_, mutable_buffers_1(trash_, 1),
                 co::bind(&MyBobStrat::HandleRead, this, _1, _2),
                 IocForCbk());
  }
  void HandleRead(const VlanError& vlerr, size_t bytes_read) {
    DCHECK(vlerr.GetErrc() == VlanErrc::eof);
    DCHECK(bytes_read == 0);
    syslog(_INFO) << "Bob got EOF, closing\n";
    API().VnClose(hConn_);
  }
private:
  vlhandle_t hConn_{ 0 };
  char vnwritebuf_[1024];
  char trash_[1];
};
}

// ------------------------------------------------------------------------------------------------------------------------------------------------

void test_vlan_native_ioc(TestInfo&) {
  syslog(_INFO) << "                                                       \n";
  io_context ioc;
  MyAliceStrat alice;
  MyBobStrat bob;
  StrategyTester vtester(ioc, alice, bob, co::common_config::kMaxChunkBodySize);
  RefTrackerContext rtctx;
  vtester.Initiate(RefTracker(CUR_LOC(), rtctx.GetHandle(), []() {
    syslog(_INFO) << "ts stopped\n";
              }));
  ioc.run();
  syslog(_INFO) << "ioc.run() returned\n";
}

