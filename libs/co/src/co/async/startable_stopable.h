#pragma once

#include "co/base/ref_tracker.h"

namespace co {
namespace async {

class Startable {
public:
  virtual ~Startable() = default;

  virtual void Start(RefTracker rt) = 0;
};

class Stopable {
public:
  virtual ~Stopable() = default;

  virtual void StopUnsafe() = 0;
};

class ThreadsafeStopable {
public:
  virtual ~ThreadsafeStopable() = default;

  virtual void StopThreadsafe() = 0;
};

}}


