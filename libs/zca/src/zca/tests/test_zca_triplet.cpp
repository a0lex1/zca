#include "zca/modules/dummy/dummy_module.h"

#include "zca/triplet.h"

#include "co/net/endpoint.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace co;
using namespace co::async;

namespace {

// Connected to triplet through events by calling code
// Logs event counts, initiates stop from OnClientHandshakeReplyReceived
class TripletTester : public core::CcServerEventsLink, public core::CcClientEventsLink,
  public TripletEvents
{
public:
  virtual ~TripletTester() {}

  // public
  void SetTriplet(Triplet& triplet) {
    triplet_ = &triplet;
  }

  // [CcServerEvents impl]
  void OnBotHandshakeComplete(Shptr<cc::ICcBot> bot)  override {
    CcServerEventsLink::OnBotHandshakeComplete(bot);
    on_bot_hshake_complete_++;
  }
  void OnBotRemovedFromList(Shptr<cc::ICcBot> bot)  override {
    CcServerEventsLink::OnBotRemovedFromList(bot);
    on_bot_removed_++;
  }
  // [CcClientEvents impl]
  void OnClientHandshakeWritten() override {
    CcClientEventsLink::OnClientHandshakeWritten();
    on_client_hshake_writ_++;
  }
  void OnClientHandshakeReplyReceived()  override {
    // When client receives handshake reply, initiate stop procedure
    CcClientEventsLink::OnClientHandshakeReplyReceived();
    on_client_hshake_reply_recv_++;
    triplet_->GetAgent().StopThreadsafe();
  }
  // [TripletEvents impl]
  void OnTripletStarted() override {
    on_triplet_started_++;
  }
  void OnAgentConnectionFailed(Errcode e) override {
    on_agent_connection_failed_++;
  }
  void OnAgentIoEnded() override {
    on_agent_io_ended_++;
    triplet_->GetBackend().StopThreadsafe();
  }
  void OnBackendIoEnded() override {
    on_backend_io_ended_++;
  }
  void OnFrontendIoEnded() override {
    on_frontend_io_ended_++;
  }
public: // all public
  int on_triplet_started_{ 0 };
  int on_agent_connection_failed_{ 0 };
  int on_bot_hshake_complete_{ 0 };
  int on_bot_removed_{ 0 };
  int on_client_hshake_writ_{ 0 };
  int on_client_hshake_reply_recv_{ 0 };
  int on_agent_io_ended_{ 0 };
  int on_backend_io_ended_{ 0 };
  int on_frontend_io_ended_{ 0 };
  Triplet* triplet_{ nullptr };
};
}

void test_zca_triplet(TestInfo&) {  
  ThreadModel tm;
  RefTrackerContext rtctx(CUR_LOC());
  TripletTester tester;
  Triplet triplet(tm, static_cast<TripletEvents*>(&tester));

  tester.SetTriplet(triplet);

  triplet.EnableBackend(static_cast<core::CcServerEventsLink*>(&tester));
  triplet.EnableAgent(static_cast<core::CcClientEventsLink*>(&tester));
  
  triplet.PrepareToStartNofail();
  
  triplet.Start(RefTracker(CUR_LOC(), rtctx.GetHandle(), [] () {
    syslog(_INFO) << "Triplet i/o ended, bitech\n";
  }));
    
  tm.Run();
  if (rtctx.GetAtomicRefTrackerCount() != 0) {
    rtctx.DisableOnReleaseCalls();
    triplet.CleanupAbortedStop();
  }

  DCHECK(1== tester.on_triplet_started_);
  DCHECK(0 == tester.on_agent_connection_failed_);
  DCHECK(1 == tester.on_client_hshake_writ_);
  DCHECK(1 == tester.on_bot_hshake_complete_);
  DCHECK(1 == tester.on_client_hshake_reply_recv_);
  DCHECK(1 == tester.on_bot_removed_);
  DCHECK(1 == tester.on_agent_io_ended_);
  DCHECK(1 == tester.on_backend_io_ended_); //<< See start logic (sequence) in Triplet::Start
}


// Test with connect error. Spoil agent's destination (backend) IP address
// Use the same helper class TripletTester
void test_zca_triplet_connerror(TestInfo& ti) {
  ThreadModel tm;
  RefTrackerContext rtctx(CUR_LOC());
  TripletTester tester;
  Triplet triplet(tm, static_cast<TripletEvents*>(&tester));

  tester.SetTriplet(triplet);

  triplet.EnableBackend(static_cast<core::CcServerEventsLink*>(&tester));
  triplet.EnableAgent(static_cast<core::CcClientEventsLink*>(&tester));

  triplet.PrepareToStartNofail();


  /* <<< NOW SPOIL THE AG CONNECTION ADDRESS >>> */
  // We wan't it to be real, but erroneous
  triplet.GetAgent().SetBackendCcRemoteAddr(co::net::TcpEndpoint("255.255.255.255:8080"));


  triplet.Start(RefTracker(CUR_LOC(), rtctx.GetHandle(), []() {
    syslog(_INFO) << "Triplet i/o ended, bitech\n";
    }));

  tm.Run();
  if (rtctx.GetAtomicRefTrackerCount() != 0) {
    rtctx.DisableOnReleaseCalls();
    triplet.CleanupAbortedStop();
  }

  DCHECK(1 == tester.on_triplet_started_);
  DCHECK(1 == tester.on_agent_connection_failed_); //<< THIS FIELD SIGNALS US 'AG CONNECTION FAILED'
  DCHECK(0 == tester.on_client_hshake_writ_);
  DCHECK(0 == tester.on_bot_hshake_complete_);
  DCHECK(0 == tester.on_client_hshake_reply_recv_);
  DCHECK(0 == tester.on_bot_removed_);
  DCHECK(1 == tester.on_agent_io_ended_);
  DCHECK(1 == tester.on_backend_io_ended_); //<< See start logic (sequence) in Triplet::Start
}

