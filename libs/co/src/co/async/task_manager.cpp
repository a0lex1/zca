#include "co/async/task_manager.h"
#include "co/async/wrap_post.h"
#include "co/base/strings.h"

#include "co/xlog/xlog.h"

#include <iomanip>

#define llog() Log(_DBG) << "Taskmgr " << SPTR(this) << " `" << GET_DEBUG_TAG(*this) << "` "

using namespace std;

namespace co {
namespace async {

DEFINE_XLOGGER_SINK("taskmgr", gCoAsyncTaskmgrLogSink);
#define XLOG_CURRENT_SINK gCoAsyncTaskmgrLogSink

std::atomic<uint64_t> _dbgNumTaskmgrWorksCreated{0};
std::atomic<uint64_t> _dbgNumTaskmgrWorksDeleted{0};

TaskImpl::~TaskImpl()
{
  // TODO: i/o cleanuped
  DCHECK((!bools_.started_ && !bools_.stopped_) || (bools_.started_ && bools_.stopped_));
}

TaskImpl::TaskImpl(Shptr<Strand> strand) :
  Task(strand), tss_impl_(*this, GetFiberStrandShptr())
{

}

void TaskImpl::StopThreadsafe() {
  tss_impl_.StopThreadsafe();
}

void TaskImpl::StopUnsafe() {
  DCHECK(bools_.started_);
  bools_.stopped_ = true;
  StopUnsafeExtra();
}

void TaskImpl::Start(RefTracker rt) {
  DCHECK(!bools_.started_);

  tss_impl_.BeforeStartWithIoEnded(rt, rt);

  bools_.started_ = true;

  RefTracker rt_all(CUR_LOC(),
                    [&] () {
    // nothing
  }, rt);

  BeginIo(rt_all);
}


// ---------------------------------------------------------------------------

TaskManager::~TaskManager() {
  if (bools_.started_) {
    DCHECK(bools_.all_io_ended_ || bools_.aborted_stop_cleanuped_);
  }
  else {
    DCHECK(!bools_.all_io_ended_);
  }
  llog() << "~~~DTOR~~~\n";
}

TaskManager::TaskManager(Shptr<Strand> strand, io_context* ioc_for_work)
  :
  Fibered(strand),
  tss_impl_(*this, GetFiberStrandShptr()),
  ioc_for_work_(ioc_for_work)
{
  llog() << " CTOR\n";
}

// Begin accepting new tasks
void TaskManager::Start(RefTracker rt)
{
  llog() << "start\n";

  bools_.stop_acpt_tasks_ = false;
  bools_.started_ = true;
  _dbg_start_count_ += 1;

  if (ioc_for_work_ != nullptr) {
    // While we 'running' (accepting tasks) there is no real i/o on io_context.
    // Simulate this.
    simulated_work_ = make_unique<io_context::work>(*ioc_for_work_);
    _dbgNumTaskmgrWorksCreated += 1;
  }

  RefTracker rt_all(CUR_LOC(),
                    [&] () {

    bools_.all_io_ended_ = true;
    tss_impl_.OnIoEnded();

    if (simulated_work_) {
      llog() << "; rt_all: Deleting ioc work\n";
      simulated_work_ = nullptr;
      _dbgNumTaskmgrWorksDeleted += 1;
    }
    else {
      llog() << "; rt_all: no ioc work to delete\n";
    }
  }, rt);

  // Save last ref
  tss_impl_.BeforeStart(rt_all);
}

void TaskManager::StopThreadsafe()
{
  llog() << "StopThreadsafe \n";
  tss_impl_.StopThreadsafe();
}

void TaskManager::StopUnsafe()
{
  DCHECK(bools_.started_);

  //DCHECK(IsInsideFiberStrand()); // you know. it can be before/after io, not from StopThreadsafe post
  llog() << "StopUnsafe (now have " << task_list_.size() << " tasks)\n";
  // stop accepting new tasks
  bools_.stop_acpt_tasks_ = true;
  // task list can't change, we are in fiber
  for (auto& task : task_list_) {
    task->StopThreadsafe();
  }
}

// thread safe
void TaskManager::ExecuteTask(Shptr<Task> task, co::RefTracker rt_user)
{
  Shptr<RefTracker> our_last_ref_sh(tss_impl_.InterlockedLoadLastRef());
  if (our_last_ref_sh == nullptr) {

    // already stopped/stopping
    llog() << ".Task " << SPTR(task.get()) << " <!! last_ref already gone [`" << GET_DEBUG_TAG(*task) << "`]>\n";
    return;
  }
  else {
    // obtained last ref interlockedly. means we won't be destroyed until this
    // task is complete
    llog() << ".Task " << SPTR(task.get()) << " <posting add start [task `" << GET_DEBUG_TAG(*task) << "`]>\n";

    RefTracker our_last_ref(*our_last_ref_sh.get());
    DCHECK(!our_last_ref.IsEmpty());

    // Coupled rt_user and our_last_ref
    RefTracker rt_coupled(CUR_LOC(), [rt_user]() {
      (rt_user.IsEmpty()); // keep ref
      // #ReleaseRule1: Captured rt_user is about to be released BEFORE our_last_ref
                  },
                  our_last_ref);

    // LOCATION STACKTRACE
    boost::asio::post(GetFiberStrand(), co::bind(&TaskManager::AddAndStartTask,
                      this, task, rt_coupled));
  }
}

void TaskManager::CleanupAbortedStop() {
  simulated_work_ = nullptr;
  bools_.aborted_stop_cleanuped_ = true;
}

void TaskManager::AddAndStartTask(Shptr<Task> task, RefTracker rt_coupled)
{
  DCHECK(IsInsideFiberStrand());
  DCHECK(bools_.started_);
  if (bools_.stop_acpt_tasks_) {
    // don't accept new tasks
    llog() << ".Task " << SPTR(task.get()) << " < stop_ is true!>\n";
    return;
  }
  task_list_.push_back(task);

  // rt_new is attached to rt_coupled indirectly, through wrap_post-bound handler
  RefTracker rt_new(CUR_LOC(), rt_coupled.GetContextHandle(), // rt_coupled already captured below
    co::async::wrap_post(GetFiberStrand(),
        co::bind(&TaskManager::OnTaskIoEnded, this, task, rt_coupled)));

  llog() << ".Task " << SPTR(task.get()) << " < starting >\n";
  task->Start(rt_new);
}

void TaskManager::OnTaskIoEnded(Shptr<Task> task, RefTracker /*rt_coupled*/)
{
  DCHECK(IsInsideFiberStrand());
  llog() << ".Task " << SPTR(task.get()) << " < #ioended >\n";
  RemoveTask(task);
  // rt_coupled is gonna be released
}

void TaskManager::RemoveTask(Shptr<Task> task)
{
  DCHECK(IsInsideFiberStrand());
  size_t old_size = task_list_.size();
  DCHECK(old_size != 0);

  llog() << ".Task " << SPTR(task.get())
         << " < removing (" << task_list_.size() << " left) >\n";

  task_list_.remove(task);
  size_t new_size = task_list_.size();
  DCHECK(old_size - 1 == new_size); //ensure 1 removed
}

}}

