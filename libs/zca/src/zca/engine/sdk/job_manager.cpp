#include "zca/engine/sdk/job_manager.h"
#include "zca/engine/sdk/dispatch_cmd_task.h"

using namespace std;

namespace engine {

BasicJobManager::BasicJobManager(Uptr<TaskManager> underlying_taskmgr)
  :
  underlying_taskmgr_(move(underlying_taskmgr))
{

}

void BasicJobManager::CleanupAbortedStop() {
  underlying_taskmgr_->CleanupAbortedStop();
}

void BasicJobManager::ExecuteJob(Shptr<Job> job, co::RefTracker rt)
{
  SetJobId(*job.get(), cur_job_id_++);
  underlying_taskmgr_->ExecuteTask(job, rt);
}

void BasicJobManager::ListJobs()
{
  NOTREACHED();
}

Job::Job(Shptr<DispatchTaskSharedData> tshdata, Shptr<Strand> strand) :
  Task(strand),
  tshdata_(tshdata)
{

}

}


