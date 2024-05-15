#include "co/async/sync/sync_event.h"
#include "co/async/startable_stopable.h"

#include "co/base/async_coro.h"
#include "co/base/tests.h"
#include "co/base/strings.h"

#include "co/xlog/configs.h"
#include "co/xlog/xlog.h"

#include <boost/asio/deadline_timer.hpp>

using namespace co;
using namespace co::async;
using namespace co::async::sync;
using namespace std;
using namespace co::xlog;
using namespace co::xlog::configs;
using namespace boost::asio;
using namespace boost::posix_time;

void test_co_async_event(TestInfo& ti) {
  io_context ioc;
  Event ev(ioc);
  ev.AsyncWait([] () {
    syslog(_INFO) << "event signaled!\n";
  });
  ev.SetEvent();
  ioc.run();
}

void test_co_async_event_wtimer(TestInfo& ti) {
  io_context ioc;
  Event ev(ioc);
  bool event_detected = false;
  ev.AsyncWait([&event_detected] () {
    syslog(_INFO) << "event signaled!\n";
    event_detected = true;
  });
  deadline_timer dt(ioc);
  dt.expires_from_now(milliseconds(250));
  dt.async_wait([&ev] (Errcode e) {
    DCHECK(!e);
    ev.SetEvent();
  });
  ioc.run();
  DCHECK(event_detected);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace {
class TestSyncWaiter : public AsyncCoro, public Startable {
public:
  virtual ~TestSyncWaiter() = default;
  
  TestSyncWaiter(io_context& ioc)
    :
    AsyncCoro(co::bind(&TestSyncWaiter::CoroEntry, this)),
    sync_event_(make_unique<Event>(ioc), *this/*coro*/)
  {
  }
  void Start(RefTracker rt) override {
    rt_ = rt;
    syslog(_INFO) << "entering coro...\n";
    Enter();
  }
  bool IsEventDetected() const { return event_detected_; }
  SyncEvent& GetSyncEvent() { return sync_event_; }

private:
  void CoroEntry() {
    // move |rt_| to stack local storage
    RefTracker rt = rt_;
    rt_ = RefTracker();
    
    syslog(_INFO) << "doing sync_event_.Wait()\n";
    sync_event_.Wait();
    syslog(_INFO) << "sync_event_.Wait() RETURNED\n";

    event_detected_ = true;
  }
private:
  SyncEvent sync_event_;
  RefTracker rt_;
  bool event_detected_{false};
};
}
void test_co_async_sync_event_wtimer(TestInfo& ti) {
  
  io_context ioc;

  TestSyncWaiter waiter(ioc);
  
  RefTrackerContext rtctx(CUR_LOC());
  waiter.Start(RefTracker(CUR_LOC(), rtctx.GetHandle(), []() {}));
  
  deadline_timer dt(ioc);
  dt.expires_from_now(milliseconds(500));
  dt.async_wait([&] (Errcode e) {
    DCHECK(!e);
    syslog(_INFO) << "setting event...\n";
    waiter.GetSyncEvent().SetEvent();
                });  
  syslog(_INFO) << "running ioc...\n";
  ioc.run();
  syslog(_INFO) << "ioc returned event...\n";
  DCHECK(waiter.IsEventDetected());  
}











