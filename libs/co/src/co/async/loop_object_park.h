#pragma once

#include "co/async/loop_object.h"
#include "co/async/fibered.h"
#include "co/async/threadsafe_stopable_impl.h"

#include "co/xlog/define_logger_sink.h"

#include <atomic>

namespace co {
namespace async {

DECLARE_XLOGGER_SINK("loopobjpark", gCoAsyncLoopObjectParkSink);

using LoopObjectFactoryFn = Func<Shptr<LoopObject>(size_t slot)>;

// Supports reset if its objects do
class LoopObjectPark
  :
  public LoopObject,
  public Fibered,
  private Stopable
{
public:
  virtual ~LoopObjectPark() = default;

  LoopObjectPark(size_t num_clients, bool restart,
                 Shptr<Strand> strand,
                 LoopObjectFactoryFn obj_fac_func);

  // [LoopObject impl]
  void PrepareToStart(Errcode&) override;
  void CleanupAbortedStop() override;
  bool IsResetSupported() const override {
    // We support it, but other objects may not
    return !(HaveUnresetableObjects());
  }
  void ResetToNextRun() override;
  void Start(co::RefTracker rt) override;
  void StopThreadsafe() override;

  bool HaveUnresetableObjects() const;

  Shptr<Strand> GetStrand() { return tss_impl_.GetStrandShptr(); }

private:
  void StartHappyObjects(RefTracker rt);
  void CreateHappyObjects();
  void StopHappyObjects();
  //void OnObjectIoEnded(size_t slot, RefTracker rt_root);
  void OnObjectIoEndedUnsafe(size_t slot, RefTracker rt_root);
  void DisableRestarting();

  // [Stopable impl]
  void StopUnsafe() override;

  bool IsInsideStrand() {
    DCHECK(tss_impl_.GetStrandShptr() == GetFiberStrandShptr());
    return GetFiberStrandShptr()->running_in_this_thread();
  }

private:
  ThreadsafeStopableImpl tss_impl_;
  size_t num_clients_;
  std::atomic<bool> restart_;
  LoopObjectFactoryFn obj_fac_func_;
  std::vector<Shptr<LoopObject>> objects_;
};

}}
