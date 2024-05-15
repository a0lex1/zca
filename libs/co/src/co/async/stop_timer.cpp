#include "co/async/stop_timer.h"
#include "co/async/wrap_post.h"

#include "co/xlog/xlog.h"

using boost::posix_time::time_duration;
using namespace std;
using namespace co;
using namespace co::async;

namespace co {
  namespace async {

// A helper class to stop any object after waiting some time

StopTimer::StopTimer(io_context& ioc,
                     Shptr<Strand> strand,
                     ThreadsafeStopable& target_object,
                     const time_duration& delay_from_now)
  :
  ioc_(ioc),
  strand_(strand),
  asio_timer_(ioc),
  target_object_(target_object),
  delay_from_now_(delay_from_now)
{
  //ExpiresFromNow(delay_from_now);
}

void StopTimer::ExpiresFromNow(const time_duration& delay_from_now)
{
  asio_timer_.expires_from_now(delay_from_now);
}

void StopTimer::Start(co::RefTracker rt)
{
  ExpiresFromNow(delay_from_now_);
  asio_timer_.async_wait(wrap_post(*strand_.get(),
                         co::bind(&StopTimer::HandleWait, this, _1, rt)));
}

void StopTimer::HandleWait(Errcode err, RefTracker rt)
{
  if (err) {
    syslog(_DBG) << "Warning, timer `" << GET_DEBUG_TAG(*this) << "` wait failed (err " << err << ")\n";
    // IDK. Decided to stop even if |err|. Don't return.
  }
  else {
    syslog(_DBG) << "Timer `" << GET_DEBUG_TAG(*this) << "` FIRED, stopping target obj " << &target_object_ <<"\n";
  }
  target_object_.StopThreadsafe();
}

void StopTimer::StopThreadsafe()
{
  asio_timer_.cancel();
}

}}

