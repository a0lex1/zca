#include "zca/sync/sync_deadline_timer.h"

using namespace std;

void SyncDeadlineTimer::ExpiresFromNow(boost::posix_time::time_duration dur) {
  GetAdaptedObject().expires_from_now(dur);
}

Errcode SyncDeadlineTimer::Wait() {
  auto cbk(GetCoro().CreateContinuationCallback<Errcode>());
  GetCoro().DoYield([&]() {
    GetAdaptedObject().async_wait(cbk.GetFunction());
  });
  return std::get<0>(cbk.ResultTuple());
}

Errcode SyncDeadlineTimer::WaitFor(boost::posix_time::time_duration dur) {
  ExpiresFromNow(dur);
  return Wait();
}






