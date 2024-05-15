#pragma once

#include "zca/sync/sync_deadline_timer.h"
#include "zca/triplet.h"
#include "zca/zca_common_config.h"

#include "netshell/sync/sync_para_command_result_reader.h"
#include "netshell/sync/sync_command_result_reader.h"
#include "netshell/sync/sync_command_writer.h"

#include "co/async/sync/sync_event.h"
#include "co/async/threadsafe_stopable_impl.h"
#include "co/async/event_waiter.h"
#include "co/base/async_coro.h"

DECLARE_XLOGGER_SINK("zcatestobj", gZcaTestObjectLogSink);
DECLARE_XLOGGER_SINK("zcatestobjsync", gZcaTestObjectSyncLogSink);

// CHANGE HISTORY:
//   +Multiple backcli/frontcli clients
//   +Netshell client should be in ZcaTestObject; ZcaTestObjectSync should contain only sync-to-async wrapping code
//   -Remove ZcaTestObjectSync, rewrite all tests to async model

class ZcaTestObject
  :
  public co::async::LoopObjectNoreset,
  private TripletEvents,
  private co::async::Stopable,
  private core::CcServerEventsLink,
  private core::CcClientEventsLink
{
public:
  virtual ~ZcaTestObject();

  using RefTracker = co::RefTracker;
  using ThreadModel = co::async::ThreadModel;
  using Event = co::async::Event;
  using EventWaiter = co::async::EventWaiter;
  using ThreadsafeStopableImpl = co::async::ThreadsafeStopableImpl;
  using deadline_timer = boost::asio::deadline_timer;

  ZcaTestObject(
    ThreadModel& tm,
    size_t netshell_max_cmd_len = netshell::kNsMaxLineLen);

  // [LoopObject impl] protected
  void PrepareToStart(Errcode&) override;
  void CleanupAbortedStop() override;
  void Start(RefTracker rt) override;
  void StopThreadsafe() override;

protected:
  // [Stopable impl]
  void StopUnsafe() override;

  // [TripletEvents impl]
  void OnTripletStarted() override;
  void OnAgentConnectionFailed(Errcode err) override;
  void OnBackendIoEnded() override;
  void OnAgentIoEnded() override;
  void OnFrontendIoEnded() override;

  // [CcServerEventsLink::CcServerEvents impl]
  void OnBotHandshakeComplete(Shptr<cc::ICcBot> bot) override;
  void OnBotRemovedFromList(Shptr<cc::ICcBot> bot) override;

  // [CcClientEventsLink::CcClientEvents impl]
  void OnClientHandshakeWritten() override;
  void OnClientHandshakeReplyReceived() override;

  // <<< START YOUR TEST FROM HERE >>>
  virtual void OnAllConnected(RefTracker) {}

private:
  // implement to customize behavior
  virtual void EnableParties() = 0;
  virtual void AddModules() = 0;
  virtual void SetOptions() { /* empty */ }

protected:
  // customization can use this funcs
  void EnableBackend();
  void EnableAgent();
  void EnableFrontend();
  void EnableBackParaNetshellClient(size_t num_sessions=1);
  void EnableFrontNetshellClient(size_t num_sessions=1);
  size_t NumBackshellsEnabled() const { return backcli_num_sessions_; }
  size_t NumFrontshellsEnabled() const { return frontcli_num_sessions_; }
  //bool IsBackNetshellClientEnabled() const { return backcli_enabled_; }
  //bool IsFrontNetshellClientEnabled() const { return frontcli_enabled_; }

  Triplet& GetTriplet() { return triplet_; }

  // Friendly timer. Can be used for whatever user wants.
  boost::asio::deadline_timer& AsyncAuxTimer() { return async_aux_timer_; }

  netshell::NsParaCommandResultReader& BackshellParaResReader(size_t i=0) { return *backshell_para_cmdres_rdrs_[i]; }
  netshell::NsCommandWriter& BackshellCmdWriter(size_t i=0) { return *backres_cmd_writs_[i]; }
  netshell::NsCommandResultReader& FrontshellResReader(size_t i=0) { return *frontshell_cmdres_rdrs_[i]; }
  netshell::NsCommandWriter& FrontshellCmdWriter(size_t i=0) { return *frontshell_cmd_writs_[i]; }

private:
  void HandleFrontCliConnected(Errcode, size_t, RefTracker);
  void HandleBackCliConnected(Errcode, size_t, RefTracker);
  void HandleWaitAllEvents(RefTracker);

protected:
  Uptr<co::async::Service> service_;

private:
  ThreadsafeStopableImpl tss_impl_;
  size_t backcli_num_sessions_{ 0 }, frontcli_num_sessions_{ 0 };
  Errcode backcli_conn_err_, frontcli_conn_err_;
  Uptr<co::async::StreamFactory> stmfac_;
  Uptr<co::async::StreamConnector> connector_;
  Uptr<co::async::Event> cli_conn_event_; // created from ZcaTestObject::Start
  Errcode hshake_err_;

  Triplet triplet_;

  Uptr<netshell::NsClientFactory> netshell_fac_;

  std::vector<Uptr<co::async::Stream>> backshell_stms_;
  std::vector<Uptr<netshell::NsParaCommandResultReader>> backshell_para_cmdres_rdrs_;
  std::vector<Uptr<netshell::NsCommandWriter>> backres_cmd_writs_;
  std::vector<Uptr<Event>> backcli_conn_events_;

  std::vector<Uptr<co::async::Stream>> frontshell_stms_;
  std::vector<Uptr<netshell::NsCommandResultReader>> frontshell_cmdres_rdrs_;
  std::vector<Uptr<netshell::NsCommandWriter>> frontshell_cmd_writs_;
  std::vector<Uptr<Event>> frontcli_conn_events_;

  EventWaiter waiter_;

  deadline_timer async_aux_timer_;
  size_t netshell_max_cmd_len_;
};

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------

// A variant of sync ZcaTestObject, e.g. an object with SYNC wrappers (hosts async coro)

class ZcaTestObjectSync
  :
  public co::AsyncCoro,
  public ZcaTestObject
{
public:
  virtual ~ZcaTestObjectSync() = default;

  using RefTracker = co::RefTracker;
  using RefTrackerContext = co::RefTrackerContext;
  using ThreadModel = co::async::ThreadModel;
  using Event = co::async::Event;
  using SyncEvent = co::async::sync::SyncEvent;

  ZcaTestObjectSync(
    ThreadModel& tm,
    size_t netshell_max_cmd_len = netshell::kNsMaxLineLen);

  void PrepareToStart(Errcode&) override;
  void Start(RefTracker rt) override;
  void CleanupAbortedStop() override;

private:
  // HIDE Async versions of funcs
  using ZcaTestObject::AsyncAuxTimer;
  using ZcaTestObject::BackshellParaResReader;
  using ZcaTestObject::BackshellCmdWriter;
  using ZcaTestObject::FrontshellResReader;
  using ZcaTestObject::FrontshellCmdWriter;

protected:
  // THESE Sync versions should be used
  SyncDeadlineTimer& SyncAuxTimer() { return *sync_auxtimer_; }
  netshell::sync::SyncNsParaCommandResultReader& SyncBackshellParaResReader(size_t i=0) { return *sync_backshell_para_cmdres_rdrs_[i]; }
  netshell::sync::SyncNsCommandWriter& SyncBackshellCmdWriter(size_t i = 0) { return *sync_backshell_cmd_writs_[i]; }
  netshell::sync::SyncNsCommandResultReader& SyncFrontshellResReader(size_t i = 0) { return *sync_frontshell_cmdres_rdrs_[i]; }
  netshell::sync::SyncNsCommandWriter& SyncFrontshellCmdWriter(size_t i = 0) { return *sync_frontshell_cmd_writs_[i]; }

  // <<< START YOUR SYNC TEST FROM HERE >>>
  virtual void OnSyncAllConnected() {}

private:
  // internal
  void CoroEntry();

  // [ZcaTestObject impl]
  void OnAllConnected(RefTracker) final;


private:
  RefTracker rt_copy_;
  Uptr<SyncEvent> onallconnected_event_;
  Uptr<SyncDeadlineTimer> sync_auxtimer_;
  std::vector<Uptr<netshell::sync::SyncNsParaCommandResultReader>> sync_backshell_para_cmdres_rdrs_;
  std::vector<Uptr<netshell::sync::SyncNsCommandWriter>> sync_backshell_cmd_writs_;
  std::vector<Uptr<netshell::sync::SyncNsCommandResultReader>> sync_frontshell_cmdres_rdrs_;
  std::vector<Uptr<netshell::sync::SyncNsCommandWriter>> sync_frontshell_cmd_writs_;
};




