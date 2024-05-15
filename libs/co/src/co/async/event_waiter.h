#pragma once

#include "co/async/event.h"

#include <vector>

namespace co {
namespace async {

class EventWaiter {
public:
  // vector copied
  void WaitAllEvents(const std::vector<Event*>& events, Func<void()> handler);

private:
  void DoNextWait();
  void HandleWait();
  void ClearAndCallHandler();

private:
  std::vector<Event*> events_;
  Func<void()> handler_;
  size_t cur_event_{ 0 };
};

}}
