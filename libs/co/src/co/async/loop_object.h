#pragma once

#include "co/async/cleanupable.h"
#include "co/async/thread_model.h"
#include "co/async/startable_stopable.h"

namespace co {
namespace async {

class LoopObjectBase : public Cleanupable {
public:
  virtual ~LoopObjectBase() = default;

  // Rule: PrepareToStart() can use network (bind acceptors, etc.) and
  // throw any exceptions as well as Run(), BUT PrepareToStart() should NOT create new i/o
  // new i/o must be created only from Start()
  virtual void PrepareToStart(Errcode& err) = 0;
  //Cleanupable::CleanupAbortedStop();
  virtual bool IsResetSupported() const = 0;
  virtual void ResetToNextRun() = 0;

  // exception throwing pukers
  void PrepareToStartNofail() {
    Errcode e;
    PrepareToStart(e);
    if (e) {
      BOOST_THROW_EXCEPTION(boost::system::system_error(e));
    }
  }
};

class LoopObject
  :
  public LoopObjectBase, // +Cleanupable
  public Startable,
  public ThreadsafeStopable
{
public:
  virtual ~LoopObject() = default;

  //Start(RefTracker)
  //StopThreadsafe()
  //CleanupAbortedStop()
};

class LoopObjectNoreset : public LoopObject {
public:
  virtual ~LoopObjectNoreset() = default;

  bool IsResetSupported() const  final { return false; }
  void ResetToNextRun()  final { /* nothing */ }
};

class EmptyLoopObject : public LoopObject {
public:
  virtual ~EmptyLoopObject() = default;

  // [LoopObjectBase impl]
  void PrepareToStart(Errcode& e) override {
  }
  void CleanupAbortedStop() override {
  }
  bool IsResetSupported() const override {
    return true;
  }
  void ResetToNextRun() override {
  }

  // [LoopObject impl]
  void Start(RefTracker) override {

  }

  void StopThreadsafe() override {

  }
};


}}
