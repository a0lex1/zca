#include "zca/test_kit/zca_test_object.h"
#include "zca/netshell_status_descriptor_table.h"
#include "zca/zca_common_config.h"
#include "zca/netshell_status_descriptor_table.h"

#include "co/async/tcp_service.h"
#include "co/xlog/xlog.h"

#include <boost/exception/exception.hpp>

#define llogi() Log(_INFO) << "<ZcaTestObj> "

using namespace std;
using namespace co;
using namespace co::async;
using namespace co::async::sync;

DEFINE_XLOGGER_SINK("zcatestobj", gZcaTestObjectLogSink);
#define XLOG_CURRENT_SINK gZcaTestObjectLogSink

ZcaTestObject::~ZcaTestObject() {
  Log(_TRACE) << "DTOR\n";
}

ZcaTestObject::ZcaTestObject(ThreadModel& tm, size_t netshell_max_cmd_len)
  :
  service_(make_unique<TcpService>(tm.DefIOC())),
  tss_impl_(*this, make_shared<Strand>(service_->GetIoContext())),
  triplet_(tm, static_cast<TripletEvents*>(this)),
  async_aux_timer_(tm.DefIOC()),
  netshell_max_cmd_len_(netshell_max_cmd_len)
{
  netshell_fac_ = make_unique<netshell::NsClientFactoryText>();
  Log(_TRACE) << "CTOR\n";
}

void ZcaTestObject::PrepareToStart(Errcode& err) {
  EnableParties();
  AddModules();
  SetOptions();
  triplet_.PrepareToStart(err);
  if (err) {
    return;
  }

  if (triplet_.IsAgentEnabled()) {
    cli_conn_event_ = make_unique<Event>(service_->GetIoContext());
  }

  if (backcli_num_sessions_!=0 || frontcli_num_sessions_!=0) {
    stmfac_ = service_->CreateStreamFactory();
    connector_ = service_->CreateStreamConnectorFactory()->CreateStreamConnector();
  }


  for (size_t i=0; i<backcli_num_sessions_; i++) {
    // <<< FRONTEND <-> BACKEND NETSHELL IS PARALLEL >>>
    backcli_conn_events_.emplace_back(make_unique<Event>(service_->GetIoContext()));
    backshell_stms_.emplace_back(stmfac_->CreateStream());

    backshell_para_cmdres_rdrs_.emplace_back(
      netshell_fac_->CreateParallelCommandResultReader(
        gZcaNsStatusDescriptorTable,
        tss_impl_.GetStrandShptr(),
        *backshell_stms_.back(),
        netshell_max_cmd_len_));

    backres_cmd_writs_.emplace_back(
      netshell_fac_->CreateCommandWriter(
        tss_impl_.GetStrandShptr(),
        *backshell_stms_.back()
        ));
  }


  for (size_t i=0; i<frontcli_num_sessions_; i++) {
    // <<< USER <-> FRONTEND NETSHELL IS SEQUENTIAL >>>
    frontcli_conn_events_.emplace_back(make_unique<Event>(service_->GetIoContext()));
    frontshell_stms_.emplace_back(stmfac_->CreateStream());

    frontshell_cmdres_rdrs_.emplace_back(netshell_fac_->CreateCommandResultReader(
      gZcaNsStatusDescriptorTable,
      tss_impl_.GetStrandShptr(),
      *frontshell_stms_.back(),
      netshell_max_cmd_len_));

    frontshell_cmd_writs_.emplace_back(
      netshell_fac_->CreateCommandWriter(
        tss_impl_.GetStrandShptr(),
        *frontshell_stms_.back()));
  }
}

void ZcaTestObject::CleanupAbortedStop() {
  triplet_.CleanupAbortedStop();
}

void ZcaTestObject::Start(RefTracker rt) {
  tss_impl_.BeforeStartWithIoEnded(rt, rt);

  // Start the triplet before starting netshell clients; it will acquire the port(s)
  // so our netshell clients will success to connect (to backend and frontend, correspondingly).
  triplet_.Start(rt);

  vector<Event*> events_to_wait;

  if (triplet_.IsAgentEnabled()) {
    events_to_wait.push_back(cli_conn_event_.get());
  }

  for (size_t i = 0; i < backshell_stms_.size(); i++) {
    connector_->AsyncConnect(
      triplet_.GetBackend().GetLocalAddressToConnect(),
      *backshell_stms_[i],
      co::bind(&ZcaTestObject::HandleBackCliConnected, this, _1, i, rt));

    events_to_wait.push_back(backcli_conn_events_[i].get());
  }

  for (size_t i = 0; i < frontshell_stms_.size(); i++) {
    connector_->AsyncConnect(
      triplet_.GetFrontend().GetLocalAddressToConnect(),
      *frontshell_stms_[i],
      co::bind(&ZcaTestObject::HandleFrontCliConnected, this, _1, i, rt));

    events_to_wait.push_back(frontcli_conn_events_[i].get());
  }

  // should COPY events from vector!!
  waiter_.WaitAllEvents(events_to_wait,
    co::async::wrap_post(tss_impl_.GetStrand(),
      co::bind(&ZcaTestObject::HandleWaitAllEvents, this, rt)));
}

void ZcaTestObject::HandleWaitAllEvents(RefTracker rt) {
  //
  //
  //
  OnAllConnected(rt);
}

void ZcaTestObject::StopThreadsafe() {
  tss_impl_.StopThreadsafe();
}

void ZcaTestObject::StopUnsafe() {
  triplet_.StopThreadsafe();

  // Set events to abandon waiting
  if (cli_conn_event_) {
    cli_conn_event_->SetEvent();
  }
  for (size_t i=0; i<backcli_conn_events_.size(); i++) {
    backcli_conn_events_[i]->SetEvent();
  }
  for (size_t i = 0; i < frontcli_conn_events_.size(); i++) {
    frontcli_conn_events_[i]->SetEvent();
  }

  // NOT THREADSAFE OPERATIONS
  for (size_t i=0; i<backshell_stms_.size(); i++) {
    backshell_stms_[i]->Close();
  }
  for (size_t i=0; i<frontshell_stms_.size(); i++) {
    frontshell_stms_[i]->Close();
  }
}

void ZcaTestObject::HandleBackCliConnected(Errcode err, size_t nsession, RefTracker rt) {
  if (err) {
    backcli_conn_err_ = err;
    StopThreadsafe();
    return;
  }
  backcli_conn_events_[nsession]->SetEvent();
}

void ZcaTestObject::HandleFrontCliConnected(Errcode err, size_t nsession, RefTracker rt) {
  if (err) {
    frontcli_conn_err_ = err;
    StopThreadsafe();
    return;
  }
  frontcli_conn_events_[nsession]->SetEvent();
}

void ZcaTestObject::OnTripletStarted() {
}

void ZcaTestObject::OnAgentConnectionFailed(Errcode err) {
  hshake_err_ = err;
}

void ZcaTestObject::OnBackendIoEnded() {
}

void ZcaTestObject::OnAgentIoEnded() {
}

void ZcaTestObject::OnFrontendIoEnded() {
}

void ZcaTestObject::OnBotHandshakeComplete(Shptr<cc::ICcBot> bot) {
  // call base
  CcServerEventsLink::OnBotHandshakeComplete(bot);
}

void ZcaTestObject::OnBotRemovedFromList(Shptr<cc::ICcBot> bot) {
  // call base
  CcServerEventsLink::OnBotRemovedFromList(bot);
}

void ZcaTestObject::OnClientHandshakeWritten() {
  // call base
  CcClientEventsLink::OnClientHandshakeWritten();
}

void ZcaTestObject::OnClientHandshakeReplyReceived() {
  // call base
  CcClientEventsLink::OnClientHandshakeReplyReceived();
  // Someone is maybe waiting for us
  cli_conn_event_->SetEvent();
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ZcaTestObject::EnableBackend() {
  triplet_.EnableBackend(static_cast<core::CcServerEventsLink*>(this));
}

void ZcaTestObject::EnableAgent() {
  triplet_.EnableAgent(static_cast<core::CcClientEventsLink*>(this));
}

void ZcaTestObject::EnableFrontend() {
  triplet_.EnableFrontend();
}

void ZcaTestObject::EnableBackParaNetshellClient(size_t num_sessions) {
  backcli_num_sessions_ = num_sessions;
}

void ZcaTestObject::EnableFrontNetshellClient(size_t num_sessions) {
  frontcli_num_sessions_ = num_sessions;
}


// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------
// SYNC WRAPPER --------------------------------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------

#undef XLOG_CURRENT_SINK
DEFINE_XLOGGER_SINK("zcatestobjsync", gZcaTestObjectSyncLogSink);
#define XLOG_CURRENT_SINK gZcaTestObjectSyncLogSink

ZcaTestObjectSync::ZcaTestObjectSync(ThreadModel& tm, size_t netshell_max_cmd_len)
  :
  AsyncCoro(co::bind(&ZcaTestObjectSync::CoroEntry, this)),
  ZcaTestObject(tm, netshell_max_cmd_len)
{
  // objects will be created in PrepareToStart()
}


void ZcaTestObjectSync::PrepareToStart(Errcode& err)
{
  if (GetTriplet().IsFrontendEnabled()) {
    DCHECK(GetTriplet().IsBackendEnabled());
  }
  ZcaTestObject::PrepareToStart(err);
  if (err) {
    return;
  }
  // <<< In PrepareToStart(), WRAP ASYNC objects in SYNC wrappers >>>
  sync_auxtimer_ = make_unique<SyncDeadlineTimer>(AsyncAuxTimer()/*base's async*/,
                                            static_cast<AsyncCoro&>(*this));
  onallconnected_event_ = make_unique<SyncEvent>(
    make_unique<Event>(service_->GetIoContext()),
    static_cast<AsyncCoro&>(*this));
  
  for (size_t i=0; i<NumBackshellsEnabled(); i++) {
    sync_backshell_para_cmdres_rdrs_.emplace_back(
      make_unique<netshell::sync::SyncNsParaCommandResultReader>(
        BackshellParaResReader(i), static_cast<AsyncCoro&>(*this)));

    sync_backshell_cmd_writs_.emplace_back(
      make_unique<netshell::sync::SyncNsCommandWriter>(
        BackshellCmdWriter(i), static_cast<AsyncCoro&>(*this)));
  }

  for (size_t i = 0; i < NumFrontshellsEnabled(); i++) {
    sync_frontshell_cmdres_rdrs_.emplace_back(
      make_unique<netshell::sync::SyncNsCommandResultReader>(
        FrontshellResReader(i), static_cast<AsyncCoro&>(*this)));

    sync_frontshell_cmd_writs_.emplace_back(
      make_unique<netshell::sync::SyncNsCommandWriter>(
        FrontshellCmdWriter(i), static_cast<AsyncCoro&>(*this)));
  }
}

void ZcaTestObjectSync::OnAllConnected(RefTracker rt) {
  onallconnected_event_->SetEvent();
}

void ZcaTestObjectSync::Start(RefTracker rt)
{
  rt_copy_ = rt;
  Enter(); // will call CoroEntry()
}

void ZcaTestObjectSync::CoroEntry()
{
  // Move rt copy from object's field to local var. Now holding in stack.
  RefTracker rt_user(rt_copy_);
  rt_copy_ = RefTracker();


  // start base object with triplet; will continue in OnXxx event handlers
  ZcaTestObject::Start(RefTracker(CUR_LOC(),
    []() {
      Log(_DBG) << "*** ZcaTestObject base #ioended ***\n";
    },
    rt_user));

  // To get inside the coro, wait here for OnAllConnected event
  onallconnected_event_->Wait();

  OnSyncAllConnected();

  Log(_DBG) << "Returning from CoroEntry()\n";

  // ZcaTestObject base can be still running, attached to |rt_copy|
}

void ZcaTestObjectSync::CleanupAbortedStop()
{
  ZcaTestObject::CleanupAbortedStop();
  rt_copy_ = RefTracker();
}

