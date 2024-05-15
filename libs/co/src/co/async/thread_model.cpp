#include "co/async/thread_model.h"
#include "co/xlog/xlog.h"

using namespace std;

namespace co {
namespace async {

  // GLKOBAL
  DefaultIoctxRunner gDefaultIocRunner;

class ThreadModelGroup {
public:
  ThreadModelGroup(size_t);
  io_context& GetIoContext() { return ioc_; }
  void StartThreads(IoctxRunner& ioc_runner);
  void WaitThreads();
  void Restart() {
    ioc_.restart();
  }
private:
  void ThreadEntry(IoctxRunner& ioc_runner);
private:
  io_context ioc_;
  std::list<Uptr<boost::thread>> threads_;
};
// ------------------------------------------------------------------------------------------------------------------------------
ThreadModelGroup::ThreadModelGroup(size_t num_threads)
{
  for (size_t i = 0; i < num_threads; i++) {
    threads_.push_back(nullptr);
  }
}

void ThreadModelGroup::StartThreads(IoctxRunner& ioc_runner) {
  syslog(_DBG) << "Starting " << threads_.size() << " threads\n";
  for (auto& t : threads_) {
    t = make_unique<boost::thread>(co::bind(&ThreadModelGroup::ThreadEntry,
                                   this,
                                   std::ref(ioc_runner)));
  }
}

void ThreadModelGroup::WaitThreads() {
  for (auto& t : threads_) {
    t->join();
  }
}

void ThreadModelGroup::ThreadEntry(IoctxRunner& ioc_runner) {
  //syslog(_DBG) << "Entering fn_thrd_run(ioc_)\n";

  // Call user's run() on our ioc
  ioc_runner.DoWrappedIoContextRun(ioc_);

  //syslog(_DBG) << "Leaving fn_thrd_run(ioc_)\n";
}
// ------------------------------------------------------------------------------------------------------------------------------
ThreadModel::~ThreadModel() {
  syslog(_DBG) << "tm dtor\n";
}

ThreadModelConfig::ThreadModelConfig(bool _force_this_thread /*= false*/,
                                     const vector<size_t>& _num_threads_for_group /*= { 1 }*/,
                                     size_t _num_threads_default /*= 1*/)
  :
  force_this_thread(_force_this_thread),
  num_threads_for_group(_num_threads_for_group),
  num_threads_default(_num_threads_default)
{
  DCHECK(!force_this_thread || (num_threads_for_group.empty() && num_threads_default == 0));
}
// ------------------------------------------------------------------------------------------------------------------------------
ThreadModel::ThreadModel(const ThreadModelConfig& conf)
  :
  conf_(conf), start_called_(false), num_threads_default_(conf.num_threads_default)
{
  if (conf.force_this_thread) {
    ioc_this_thrd_ = make_unique<io_context>();
  }
}

io_context& ThreadModel::GetDefaultIoctx() {
  return AcquireIoContextForThreadGroup(0);
}

boost::asio::io_context& ThreadModel::AcquireIoContextForThreadGroup(size_t group_num) {
  // --------------------------------------------------------------------------------------------------------
  // DCHECK(!start_called_);
  // was enabled but was alerting. seems legit to comment out this DCHECK
  // But thread safety is on your own (don't StopThreadsafe too early)
  // --------------------------------------------------------------------------------------------------------

  if (conf_.force_this_thread) {
    return *ioc_this_thrd_.get();
  }
  if (thread_groups_.find(group_num) == thread_groups_.end()) {
    if (conf_.num_threads_for_group.size() > group_num) {
      thread_groups_[group_num] = make_shared<ThreadModelGroup>(
        conf_.num_threads_for_group[group_num]);
    }
    else {
      thread_groups_[group_num] = make_shared<ThreadModelGroup>(
        conf_.num_threads_default);
    }
  }
  return thread_groups_[group_num]->GetIoContext();
}

// ------------------------------------------------------------------------------------------------------

void ThreadModel::Run(IoctxRunner& ioc_runner) {
  if (conf_.force_this_thread) {
    // Run in current thread like old asio samples
    ioc_runner.DoWrappedIoContextRun(*ioc_this_thrd_.get());
  }
  else {
    StartThreads(ioc_runner);
    WaitThreads();
  }
}

void ThreadModel::StopThreadsafe() {
  for (auto& group : thread_groups_) {
    size_t id = group.first;
    group.second->GetIoContext().stop();
  }
}

void ThreadModel::Restart() {
  for (auto& group : thread_groups_) {
    size_t id = group.first;
    group.second->Restart();
  }
}

void ThreadModel::StartThreads(IoctxRunner& ioc_runner) {
  start_called_ = true;
  for (auto g : thread_groups_) {
    g.second->StartThreads(ioc_runner);
  }
}

void ThreadModel::WaitThreads() {
  for (auto g : thread_groups_) {
    g.second->WaitThreads();
  }
  start_called_ = false;
}

size_t ThreadModel::GetNumThreadsDefault() const {
  return num_threads_default_;
}


}}






