#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

namespace co {
namespace async {
namespace capsule {

class Cooldowns {
public:
  virtual ~Cooldowns() = default;
  enum class WhatFor {
    kNormalExit,
    kInitPhaseException,
    kStartPhaseException,
    kRunPhaseException,
  };
  virtual void CooldownBeforeRestart(WhatFor wtf) = 0;
};

class ConservativeCooldowns : public Cooldowns {
public:
  void CooldownBeforeRestart(WhatFor wtf) override {
    switch (wtf) {
    case WhatFor::kNormalExit:
      // don't sleep, restart right now
      return;
    default:
      // not normal exit
      return boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
    }
  }
};

class MinimumCooldowns : public Cooldowns {
public:
  void CooldownBeforeRestart(WhatFor wtf) override {
    switch (wtf) {
    case WhatFor::kNormalExit:
      // don't sleep, restart right now
      return;
    default:
      // not normal exit
      return boost::this_thread::sleep(boost::posix_time::milliseconds(1));
    }
  }
};
}}}
