#pragma once

#include "co/async/loop_object.h"
#include "co/base/debug_tag_owner.h"

#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/asio/deadline_timer.hpp>

namespace co {
namespace async {

// An utility class. Very friendly. It will friend even with you.
// A helper class to stop any object after waiting some time

class StopTimer : public co::async::LoopObjectNoreset, public co::DebugTagOwner {
public:
  virtual ~StopTimer() = default;

  using ThreadsafeStopable = co::async::ThreadsafeStopable;
  using RefTracker = co::RefTracker;

  StopTimer(io_context& ioc_for_timer,
            Shptr<Strand> strand,
            ThreadsafeStopable& target_object,
            const boost::posix_time::time_duration& delay_from_now);

  // Could also be CTOR with Shptr<ThreadsafeStopable>

  void ExpiresFromNow(const boost::posix_time::time_duration& delay_from_now);

  // [LoopObject impl]
  void PrepareToStart(Errcode& err) override { }
  void CleanupAbortedStop() override { }
  void Start(co::RefTracker rt) override;
  void StopThreadsafe() override;

  io_context& GetIoContext() { return ioc_; }

private:
  void HandleWait(Errcode, RefTracker);

private:
  io_context& ioc_;
  Shptr<Strand> strand_;
  boost::asio::deadline_timer asio_timer_;
  ThreadsafeStopable& target_object_;
  boost::posix_time::time_duration delay_from_now_;
};

}}

