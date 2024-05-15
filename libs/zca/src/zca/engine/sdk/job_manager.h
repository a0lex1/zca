#pragma once

#include "co/async/task_manager.h"

namespace engine {

class JobManager;
class DispatchTaskSharedData;

class Job : public co::async::Task {
public:
  virtual ~Job() = default;

  Job(Shptr<DispatchTaskSharedData> tshdata, Shptr<Strand> strand);

protected:
  int GetJobId() const { return id_; }
  Shptr<DispatchTaskSharedData> GetTaskSharedData() { return tshdata_; }

private:
  friend JobManager;
  void SetId(int id) { id_ = id; }

private:
  Shptr<DispatchTaskSharedData> tshdata_;
  int id_{ -1 };
};

class JobManager : public co::async::Cleanupable {
public:
  virtual ~JobManager() = default;

  virtual void ExecuteJob(Shptr<Job> job, co::RefTracker) = 0;
  virtual void ListJobs() = 0;

protected:
  static void SetJobId(Job& job, int id) {
    job.SetId(id);
  }
};

class BasicJobManager : public JobManager {
public:
  virtual ~BasicJobManager() = default;

  using TaskManager = co::async::TaskManager;

  BasicJobManager(Uptr<TaskManager> underlying_taskmgr);

  // [JobManager impl]
  void CleanupAbortedStop() override;
  void ExecuteJob(Shptr<Job> job, co::RefTracker rt) override;
  void ListJobs() override;

private:
  Uptr<TaskManager> underlying_taskmgr_;
  int cur_job_id_{ 0 };
};

}



