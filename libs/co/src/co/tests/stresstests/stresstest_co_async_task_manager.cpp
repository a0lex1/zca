#include <string>

namespace tmtest {
struct TaskCommanderConfig;
}

namespace co {
  bool ConvertValue(const std::string& str, tmtest::TaskCommanderConfig& val);
}

#include "co/async/configs/thread_model_config_from_dict.h"

#include "co/async/wrap_post.h"
#include "co/async/task_manager.h"
#include "co/async/loop_object_park.h"
#include "co/async/loop_object_set.h"
#include "co/async/stop_timer.h"
#include "co/base/dict.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

#include <boost/asio/deadline_timer.hpp>

using namespace co;
using namespace co::async;
using namespace co::async;
using namespace co::async::configs;
using namespace std;
using namespace boost::asio;
using namespace boost::posix_time;

DEFINE_XLOGGER_SINK("taskmgrtest", gCoAsyncTMTestSink);
#define XLOG_CURRENT_SINK gCoAsyncTMTestSink

#define llogc() Log(_DBG) << "TCmder (slot " << slot_ << ") "
#define llogx() Log(_INFO)

namespace tmtest {

// Used by both class TaskCreationTask and class TaskCommander. However, |wait_children| is
// for TaskCommander only. Maybe others too.
struct TaskCommanderConfig {
  int depth_;
  int leafs_;
  bool repost_;
  bool wait_children_;
  size_t final_sleep;
};

class TaskCreationTask : public TaskImpl, public co::enable_shared_from_this<TaskCreationTask> {
public:
  virtual ~TaskCreationTask() = default;

  static Shptr<TaskCreationTask> Create(size_t slot,
                                        io_context& ioc,
                                        TaskManager& taskmgr,
                                        const TaskCommanderConfig& cmsconf)
  {
    return Shptr<TaskCreationTask>(
      new TaskCreationTask(slot, ioc, taskmgr, cmsconf));
  }

private:
  TaskCreationTask(size_t slot,
                   io_context& ioc,
                   TaskManager& taskmgr,
                   const TaskCommanderConfig& cmsconf)
    :
    TaskImpl(make_unique<Strand>(ioc)), slot_(slot), ioc_(ioc), taskmgr_(taskmgr),
    cmsconf_(cmsconf),
    spaces_(cmsconf_.depth_*2, ' ')
  {
  }

private:
  void BeginIo(RefTracker rt) override {
    rt.SetReferencedObject(shared_from_this());

    if (cmsconf_.depth_ == 0) {
      // recursion ends here
      if (cmsconf_.final_sleep != 0) {
        final_timer_ = make_unique<deadline_timer>(ioc_);
        Log(_DBG) << spaces_ << "TaskCrTask- " << slot_ << ", aborting BeginIo through final_sleep " << cmsconf_.final_sleep << "msec\n";

        final_timer_->expires_from_now(milliseconds(cmsconf_.final_sleep));
        final_timer_->async_wait(wrap_post(*GetTaskStrand().get(),
                                           co::bind(&TaskCreationTask::HandleFinalWait,
                                                    shared_from_this(), _1, rt)));
      }
      else {
        Log(_DBG) << spaces_ << "TaskCrTask- " << slot_ << ", aborting BeginIo, depth = 0\n";
      }
      return;
    }
    auto continuator = wrap_post(*GetTaskStrand().get(),
                                 co::bind(&TaskCreationTask::Continue, shared_from_this(), rt));
    if (cmsconf_.repost_) {
      boost::asio::post(ioc_, continuator);
    }
    else {
      continuator();
    }
  }
  void HandleFinalWait(Errcode err, RefTracker) {
    DCHECK(final_timer_);
    if (err) {
      Log(_DBG) << spaces_ << "TaskCrTask- HandleFinalWait ERROR " << err << " (ignoring)\n";
    }
    // |rt| is gonna be released, end of IO
  }
  void StopUnsafeExtra() override {
    // no way to stop, lol, just wait for it to stop itself
  }
private:
  void Continue(RefTracker rt) {
    for (int leaf = 0; leaf < cmsconf_.leafs_; leaf++) {
      // For every leaf, pass a copy of cmsconf with depth_-1. Recursion ends in BeginIo.
      TaskCommanderConfig child_cmsconf = cmsconf_;
      child_cmsconf.depth_ -= 1;

      auto child = TaskCreationTask::Create(slot_, ioc_, taskmgr_, child_cmsconf);

      RefTracker task_rt;
      if (cmsconf_.wait_children_) {
        // attached, bind to rt
        task_rt = RefTracker(CUR_LOC(),
                             [=]() {
                               Log(_DBG)
                                 << spaces_ << "TaskCrTask- " << slot_ << ", awaited #ioended[leaf " << leaf << "](repost = "
                                 << boolalpha << cmsconf_.repost_ << ")\n";
                                 // releasing |rt|
                             },
                             rt);
      }
      else {
        // detached, bind to rt's RefTrackerContext
        task_rt = RefTracker(CUR_LOC(), rt.GetContextHandle(),
                             [=, spaces=spaces_, slot=slot_, cmsconf=cmsconf_]() {
                               ((void)this->spaces_);
                               // this may be already deleted
                               syslog(_DBG)
                                 << spaces << "[d] TaskCrTask- slot " << slot << ", leaf = " << leaf << ", repost = "
                                 << boolalpha << cmsconf.repost_ << "\n";
                             });
      }
      taskmgr_.ExecuteTask(child, task_rt);
    }
  }
private:
  size_t slot_;
  io_context& ioc_;
  TaskManager& taskmgr_;
  TaskCommanderConfig cmsconf_;
  std::string spaces_;
  Uptr<boost::asio::deadline_timer> final_timer_;
};

class TaskCommander : public LoopObjectNoreset {
public:
  virtual ~TaskCommander() = default;

  TaskCommander(size_t slot, io_context& ioc, const TaskCommanderConfig& cmsconf)
    :
    slot_(slot),ioc_(ioc), cmsconf_(cmsconf), taskmgr_(make_shared<Strand>(ioc), &ioc)
  {
  }

  void PrepareToStart(Errcode&) override {}
  void CleanupAbortedStop() override {}
  void Start(RefTracker rt) override {
    taskmgr_.Start(rt);
    StartInitialTask(rt);
  }
  void StopThreadsafe() override {
    //llogc() << "StopThreadsafe\n";
    taskmgr_.StopThreadsafe();
  }
private:
  void StartInitialTask(RefTracker rt) {
    //llogc() << "creating and starting initial task\n";
    auto initial_task = TaskCreationTask::Create(slot_, ioc_, taskmgr_, cmsconf_);
    auto rt_task = RefTracker(CUR_LOC(),
                         [&]() {
                           //llogc() << "TCommander " << slot_ << " : initial task #ioended\n";
                           StopThreadsafe();
                         },
                         rt);
    taskmgr_.ExecuteTask(initial_task, rt_task);
  }

private:
  size_t slot_;
  io_context& ioc_;
  TaskCommanderConfig cmsconf_;
  TaskManager taskmgr_;
};

// ------------------

// TaskManager test options
struct Topts {
  // Fields
  size_t num_commanders{ 4 };
  bool restart_commanders{ true };
  TaskCommanderConfig commander_conf = {
    2,
    2,
    false/*repost*/,
    true/*wait_child*/,
    0/*final_sleep*/
  };

  size_t commander_stop_delay{100}; // msec

  // CTORs
  Topts() {
  }
  Topts(StringMap& sm, ConsumeAction consume = ConsumeAction::kDontConsume)
  {
    _OverrideFromDict(sm, consume);
  }
private:
  void _OverrideFromDict(StringMap& sm, ConsumeAction consume = ConsumeAction::kDontConsume) {
    OverrideFromDict<string, string, size_t>(sm, "num", num_commanders, consume);
    OverrideFromDict<string, string, bool>(sm, "restart", restart_commanders, consume);
    OverrideFromDict<string, string, TaskCommanderConfig>(sm, "cmsconf", commander_conf, consume);
    OverrideFromDict<string, string, size_t>(sm, "stop", commander_stop_delay, consume);
  }
};

static void TestTaskmgrWith(const ThreadModelConfig& tmconf, const Topts& to) {
  ThreadModel tm(tmconf);
  io_context& ioc = tm.DefIOC();

  auto cms_strand = make_shared<Strand>(ioc);
  auto cms_park = make_unique<LoopObjectPark>(
    to.num_commanders, to.restart_commanders,
    cms_strand,
    [=, &ioc](size_t slot) {
      return make_unique<TaskCommander>(slot, ioc, to.commander_conf);
    });
  time_duration stopdel_time;
  stopdel_time = milliseconds(to.commander_stop_delay);
  auto stopt = make_unique<StopTimer>(ioc, make_shared<Strand>(ioc), *cms_park.get(), stopdel_time);
  auto objset_strand = make_shared<Strand>(ioc);
  LoopObjectSet objset(objset_strand);
  objset.AddObject(move(cms_park));
  objset.AddObject(move(stopt));

  RefTrackerContext rtctx(CUR_LOC());
  RefTracker rt_objset(CUR_LOC(), rtctx.GetHandle(),
                []() {
                  llogx() << "objset #ioended\n";
                });
  objset.PrepareToStartNofail();
  objset.Start(rt_objset);
  rt_objset = RefTracker(); // drop

  tm.Run();

  rtctx._DbgLogPrintTrackedRefTrackers();
  DCHECK(!rtctx.GetAtomicRefTrackerCount());

  //llogx() << "tm.Run() returned\n";
}

}

using namespace tmtest;

/*void stresstest_co_async_task_manager_simple(TestInfo& ti) {
  ThreadModelConfigFromDict tmconf(ThreadModelConfig(), ti.opts_dict, ConsumeAction::kDontConsume);
  Topts to(ti.opts_dict, ConsumeAction::kDontConsume);
  TaskManager tm;
  tm.Start();
  tm.ExecuteTask();
}*/

void stresstest_co_async_task_manager_u(TestInfo& ti) {
  // TODO: add stop-ioc

  ThreadModelConfigFromDict tmconf(ThreadModelConfig(), ti.opts_dict, ConsumeAction::kDontConsume);
  Topts to(ti.opts_dict, ConsumeAction::kDontConsume);

  TestTaskmgrWith(tmconf, to);

}

void stresstest_co_async_task_manager1(TestInfo&) {
  for (size_t i = 0; i < 30; i++) {
    ThreadModelConfig tmconf;
    tmconf.num_threads_default = 1;
    Topts to;
    to.restart_commanders = true;
    to.num_commanders = 10;
    to.commander_stop_delay = 50;
    to.commander_conf.depth_ = 3;
    to.commander_conf.leafs_ = 3;
    to.commander_conf.repost_ = false;
    to.commander_conf.wait_children_ = false;
    TestTaskmgrWith(tmconf, to);
  }
}

void stresstest_co_async_task_manager2(TestInfo&) {

  for (size_t i = 0; i < 10; i++) {
    ThreadModelConfig tmconf;
    tmconf.num_threads_default = 24;
    Topts to;
    to.restart_commanders = true;
    to.num_commanders = 10;
    to.commander_stop_delay = 333;
    to.commander_conf.depth_ = 2;
    to.commander_conf.leafs_ = 2;
    to.commander_conf.repost_ = false;
    to.commander_conf.wait_children_ = false; // without wait_children
    to.commander_conf.final_sleep = 113;
    TestTaskmgrWith(tmconf, to);
  }
}

void stresstest_co_async_task_manager3(TestInfo&) {

  for (size_t i = 0; i < 10; i++) {
    ThreadModelConfig tmconf;
    tmconf.num_threads_default = 24;
    Topts to;
    to.restart_commanders = true;
    to.num_commanders = 10;
    to.commander_stop_delay = 333;
    to.commander_conf.depth_ = 2;
    to.commander_conf.leafs_ = 2;
    to.commander_conf.repost_ = false;
    to.commander_conf.wait_children_ = true; // with wait_children
    to.commander_conf.final_sleep = 113;
    TestTaskmgrWith(tmconf, to);
  }
}

void stresstest_co_async_task_manager_cases(TestInfo&) {
  uint64_t total = 0;
  for (size_t num_th_def : { 24 }) {
    for (size_t stop_del : {0, 1, 10, 77 }) {
      for (size_t depth : { 1, 2, 3}) {
        for (size_t leafs : {1, 2, 3}) {
          for (bool repost : {false, true}) {
            for (bool wait_children : {true, false}) {
              for (size_t final_sleep : {0, 50}) {
                ThreadModelConfig tmconf;
                tmconf.num_threads_default = num_th_def;
                Topts to;
                to.restart_commanders = true;
                to.num_commanders = 15; //!
                to.commander_stop_delay = stop_del;
                to.commander_conf.depth_ = depth;
                to.commander_conf.leafs_ = leafs;
                to.commander_conf.repost_ = repost;
                to.commander_conf.wait_children_ = wait_children;
                to.commander_conf.final_sleep = final_sleep;
                TestTaskmgrWith(tmconf, to);

                cout << "TaskMgrTest Case# " << total << " done\n";
                total += 1;
              }
            }
          }
        }
      }
    }
  }
}

namespace co {
  bool ConvertValue(const std::string& str, tmtest::TaskCommanderConfig& val) {
    // depth,leafs,repost,wait_children
    StringVector p;
    string_split(str, ",", p);
    if (p.size() != 4) {
      return false;
    }
    if (!string_to_int(p[0], val.depth_)) {
      return false;
    }
    if (!string_to_int(p[1], val.leafs_)) {
      return false;
    }

    int tmp;
    if (!string_to_int(p[2], tmp)) {
      return false;
    }
    if (tmp != 1 && tmp != 0) {
      return false;
    }
    val.repost_ = tmp == 1;

    if (!string_to_int(p[3], tmp)) {
      return false;
    }
    if (tmp != 1 && tmp != 0) {
      return false;
    }
    val.wait_children_ = tmp == 1;
    return true;
  }
}


