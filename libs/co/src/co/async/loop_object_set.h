#pragma once

#include "co/async/threadsafe_stopable_impl.h"
#include "co/async/fibered.h"
#include "co/async/loop_object.h"

#include "co/xlog/define_logger_sink.h"

namespace co {
namespace async {

DECLARE_XLOGGER_SINK("loopobjset", gCoAsyncLoopObjectSetSink);

// Supports reset if its objects do
class LoopObjectSet
  :
  public LoopObject,
  public Fibered,
  public Stopable
{
public:
  virtual ~LoopObjectSet() = default;

  LoopObjectSet(Shptr<Strand> strand);

  Shptr<Strand> GetStrandShptr();

  LoopObjectSet& AddObject(Shptr<LoopObject> lobj);

  bool HaveUnresetableObjects() const;

  // [LoopObject impl]
  void PrepareToStart(Errcode&) override;
  void CleanupAbortedStop() override;
  bool IsResetSupported() const override;
  void ResetToNextRun() override;

  void Start(co::RefTracker rt) override;
  void StopThreadsafe() override;

  Shptr<LoopObject> GetObject(size_t index) const {
    return objects_[index];
  }
  size_t GetObjectCount() const { return objects_.size(); }

private:
  // [Stopable impl]
  void StopUnsafe() override;

  void StartObjects(RefTracker);

private:
  ThreadsafeStopableImpl tss_impl_;
  std::vector<Shptr<LoopObject> > objects_;
  bool started_{ false };
};

}}

