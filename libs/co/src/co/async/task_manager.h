#pragma once

#include "co/async/cleanupable.h"
#include "co/async/fibered.h"
#include "co/async/threadsafe_stopable_impl.h"

#include "co/base/location.h" // probably, to co/base/debug/
#include "co/base/debug_tag_owner.h"

#include "co/xlog/define_logger_sink.h"

#include <boost/noncopyable.hpp>
#include <list>

namespace co {
namespace async {

DECLARE_XLOGGER_SINK("taskmgr", gCoAsyncTaskmgrLogSink);

extern std::atomic<uint64_t> _dbgNumTaskmgrWorksCreated;
extern std::atomic<uint64_t> _dbgNumTaskmgrWorksDeleted;

class TaskManager;

/**
 * class Task
 * 
 * Base for your tasks. All tasks have strand. Deriveds implements MT stopability
 * himself (see TaskImpl).
 * Friends only with class TaskManager. They deal with human trafficing.
 */
class Task:
  private co::async::Startable, // only for friend TaskManager
  protected co::async::Stopable,
  public co::async::ThreadsafeStopable,
  public co::async::Fibered,
  public co::DebugTagOwner
{
public:
  virtual ~Task() = default;

  using Fibered::Fibered; // Shptr<strand>

  //Start(rt) = 0
  //StopThreadsafe() = 0

protected:
  //StopUnsafe() = 0

  //Fibered::GetStrand()

private:
  // Task can be started and stopped only by TaskManager
  friend class TaskManager;
};

/*
* class TaskTM
* 
* A task with thread safe stop implemented, deriveds only needs to implement unsafe stop.
*/
class TaskImpl: public Task {
public:
  virtual ~TaskImpl();

  using RefTracker = co::RefTracker;

  TaskImpl(Shptr<Strand> strand);

  void StopThreadsafe() override;

  Shptr<Strand> GetTaskStrand() { return tss_impl_.GetStrandShptr(); }
  bool IsInsideTaskStrand() { return tss_impl_.GetStrand().running_in_this_thread(); }

protected:
  void Start(RefTracker rt) final;
  void StopUnsafe() final;

  // To implement by user:
  virtual void BeginIo(RefTracker rt) = 0;
  virtual void StopUnsafeExtra() = 0;


private:
  struct {
    bool started_ : 1;
    bool stopped_ : 1;
  } bools_{ false };

  // this little rofl helps us to be MT stopable
  co::async::ThreadsafeStopableImpl tss_impl_;
};

class TaskExecutor {
public:
  virtual ~TaskExecutor() = default;

  virtual void ExecuteTask(Shptr<Task> task, co::RefTracker rt) = 0;
};

/*
* class TaskManager
* 
* This class manages your tasks
* noncopyable because copyability can lead to mistake when writing &references and
* we don't care
*/
class TaskManager :
  public boost::noncopyable,
  public co::async::Fibered, //add to fibered second ctor(&strand)
  public co::async::Startable,
  public co::async::ThreadsafeStopable,
  public TaskExecutor,
  protected co::async::Stopable,
  public DebugTagOwner,
  public Cleanupable
{
public:
  virtual ~TaskManager();

  using RefTracker = co::RefTracker;

  TaskManager(Shptr<Strand> strand, io_context* io_for_work);

  // Begin accepting tasks #SimulateIo
  void Start(RefTracker rt) override;

  // Stop already running tasks and accepting new tasks
  void StopThreadsafe() override;

  // Thread safe
  // |rt| guaranteed to fire
  //    EITHER
  //      before Start()'s rt
  //    or
  //      right after return (no i/o initiated)
  //      (if currently stopping/stopped)
  void ExecuteTask(Shptr<Task> task, RefTracker rt) override;

  void CleanupAbortedStop() override;

private:
  void StopUnsafe() override;

  void AddAndStartTask(Shptr<Task>, RefTracker);
  void OnTaskIoEnded(Shptr<Task>, RefTracker);
  void RemoveTask(Shptr<Task>);

private:
  struct {
    bool stop_acpt_tasks_ : 1;
    bool started_ : 1;
    bool all_io_ended_ : 1;
    bool aborted_stop_cleanuped_  : 1;
  } bools_{false, false, false, false};

  co::async::ThreadsafeStopableImpl tss_impl_;
  std::list<Shptr<Task>> task_list_;
  //Uptr<RefTracker> rt_tasks_;
  io_context* ioc_for_work_{nullptr};
  Uptr<io_context::work> simulated_work_;

  uint32_t _dbg_start_count_{ 0 };
};

}
}

