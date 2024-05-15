#pragma once

#include "co/async/create_for_endpoint.h"

#include "co/base/async_coro.h"

#include <boost/asio/deadline_timer.hpp>

class SyncDeadlineTimer : public co::AsyncCoroAdaptor<boost::asio::deadline_timer> {
public:
  using AsyncCoro = co::AsyncCoro;
  using deadline_timer = boost::asio::deadline_timer;
  using time_duration = boost::posix_time::time_duration;

  // TODO: try replace with using AsyncCoroAdaptor::AsyncCoroAdaptor
  SyncDeadlineTimer(Uptr<deadline_timer> adapted_uptr, AsyncCoro& coro)
    :AsyncCoroAdaptor(std::move(adapted_uptr), coro)
  {
  }
  SyncDeadlineTimer(deadline_timer& adapted_ref, AsyncCoro& coro)
    :AsyncCoroAdaptor(adapted_ref, coro)
  {
  }

  void Plugin(Uptr<deadline_timer> timer);

  void ExpiresFromNow(time_duration dur);
  Errcode Wait();
  Errcode WaitFor(time_duration dur);
};

