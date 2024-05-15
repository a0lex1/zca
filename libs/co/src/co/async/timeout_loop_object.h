#pragma once

#include "co/async/loop_object.h"

#include <boost/asio/deadline_timer.hpp>

namespace co {
namespace async {

class TimeoutLoopObject : public LoopObjectNoreset {
public:
  virtual ~TimeoutLoopObject() = default;

  using RefTracker = co::RefTracker;

  TimeoutLoopObject(io_context& ioc, boost::posix_time::time_duration delay)
    :
    timer_(ioc, delay)
  {
  }
  void PrepareToStart(Errcode& e) override {
  }
  void CleanupAbortedStop() override {
  }
  void Start(RefTracker rt) override {
    timer_.async_wait([rt] (Errcode err) {
      // timeout

      (void)rt;
    });
  }
  void StopThreadsafe() override {
    timer_.cancel();
  }

private:
  boost::asio::deadline_timer timer_;
};



}}

