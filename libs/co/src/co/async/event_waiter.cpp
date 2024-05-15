#include "co/async/event_waiter.h"

namespace co {
namespace async {

void EventWaiter::WaitAllEvents(
  const std::vector<Event*>& events, Func<void()> handler)
{
  events_ = events;
  handler_ = handler;
  DoNextWait();
}

void EventWaiter::DoNextWait() {
  if (cur_event_ == events_.size()) {
    // SUCCESS
    ClearAndCallHandler();
    return;
  }
  events_[cur_event_]->AsyncWait(co::bind(&EventWaiter::HandleWait, this));
}

void EventWaiter::HandleWait() {
  cur_event_ += 1;
  DoNextWait();
}

void EventWaiter::ClearAndCallHandler() {
  auto hcopy(handler_);
  handler_ = nullptr;
  hcopy();
}

}}

