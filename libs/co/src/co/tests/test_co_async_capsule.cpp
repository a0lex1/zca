#include "co/async/test_kit/default_capsule_object_tester.h"
#include "co/base/recoverable_exception.h"
#include "co/base/unrecoverable_exception.h"
#include "co/xlog/xlog.h"

#include <boost/asio/deadline_timer.hpp>

using namespace co;
using namespace co::async;
using namespace co::async::capsule;
using namespace co::async::test_kit;

namespace {
class MyInitiator: public LoopObject {
public:
  virtual ~MyInitiator() = default;

  MyInitiator(ThreadModel& tm) : dt_(tm.DefIOC()) {

  }

  void EnableUnrecovExceptionInjectionPreparePhase() { inj_prep_unrecov_ = true; }
  void EnableRecovExceptionInjectionPreparePhase() { inj_prep_recov_ = true; }

  void EnableUnrecovExceptionInjectionStartPhase() { inj_start_unrecov_ = true; }
  void EnableRecovExceptionInjectionStartPhase() { inj_start_recov_ = true; }

  void EnableUnrecovExceptionInjectionRunPhase() { inj_run_unrecov_ = true; }
  void EnableRecovExceptionInjectionRunPhase() { inj_run_recov_ = true; }

  void EnableInvalidArgumentInjection() { inj_invalarg_ = true; }
  
private:
  void PrepareToStart(Errcode& err) override {
    if (inj_prep_recov_) {
      throw RecoverableException();
    }
    if (inj_prep_unrecov_) {
      throw UnrecoverableException();
    }
    err = NoError();
  }
  bool IsResetSupported() const override {
    return false;
  }
  void ResetToNextRun() override {
    NOTREACHED();
  }
  void CleanupAbortedStop() override {
    //
  }
  void Start(RefTracker rt) override {
    if (inj_start_recov_) {
      throw RecoverableException();
    }
    if (inj_start_unrecov_) {
      throw UnrecoverableException();
    }
    if (inj_invalarg_) {
      throw std::invalid_argument("the example of invalid_argument exception");
    }
    dt_.expires_from_now(boost::posix_time::millisec(10));
    dt_.async_wait(co::bind(&MyInitiator::HandleWait, this, _1, rt));
  }
  void StopThreadsafe() override {
    dt_.cancel();
  }

private:
  void HandleWait(Errcode err, RefTracker rt) {
    if (inj_run_recov_) {
      throw RecoverableException();
    }
    if (inj_run_unrecov_) {
      throw UnrecoverableException();
    }
    // Done
    syslog(_INFO) << "HandleWait\n";
  }
  
private:
  boost::asio::deadline_timer dt_;
  bool inj_prep_recov_{ false }, inj_prep_unrecov_{ false };
  bool inj_start_recov_{ false }, inj_start_unrecov_{ false };
  bool inj_run_recov_{ false }, inj_run_unrecov_{ false };
  bool inj_invalarg_{ false };
};
}


// This test without exception (normal run)
void test_co_async_capsule_normal_run(TestInfo& ti) {
  DefaultCapsuleObjectTester tester(ti, [](ThreadModel& tm) {
    auto obj = make_unique<MyInitiator>(tm);
    return obj;
    });
  tester.ExecuteExpect(RunLoop::LoopExitCause::kIterLimitReachedCause);
}
// ---------------------

void test_co_async_capsule_unrecov_preparephase(TestInfo& ti) {
  DefaultCapsuleObjectTester tester(ti, [](ThreadModel& tm) {
    auto obj = make_unique<MyInitiator>(tm);
    obj->EnableUnrecovExceptionInjectionPreparePhase();
    return obj;
    });
  tester.ExecuteExpect(RunLoop::LoopExitCause::kUnrecoverableExceptionCause,
    ExceptionModel::ExceptionState::kExceptionUnrecoverable);
}

void test_co_async_capsule_recov_preparephase(TestInfo& ti) {
  DefaultCapsuleObjectTester tester(ti, [](ThreadModel& tm) {
    auto obj = make_unique<MyInitiator>(tm);
    obj->EnableRecovExceptionInjectionPreparePhase();
    return obj;
    },
    2, 2);
  tester.ExecuteExpect(RunLoop::LoopExitCause::kIterLimitReachedCause,
    ExceptionModel::ExceptionState::kExceptionRecoverable);
  DCHECK(tester.GetRunLoopCounters().recovered_inits == 2);
}


// ---------------------

void test_co_async_capsule_unrecov_startphase(TestInfo& ti) {
  DefaultCapsuleObjectTester tester(ti, [](ThreadModel& tm) {
    auto obj = make_unique<MyInitiator>(tm);
    obj->EnableUnrecovExceptionInjectionStartPhase();
    return obj;
    });
  tester.ExecuteExpect(RunLoop::LoopExitCause::kUnrecoverableExceptionCause,
    ExceptionModel::ExceptionState::kExceptionUnrecoverable);
}

void test_co_async_capsule_recov_startphase(TestInfo& ti) {
  DefaultCapsuleObjectTester tester(ti, [](ThreadModel& tm) {
    auto obj = make_unique<MyInitiator>(tm);
    obj->EnableRecovExceptionInjectionStartPhase();
    return obj;
    },
    2, 2);
  tester.ExecuteExpect(RunLoop::LoopExitCause::kIterLimitReachedCause,
    ExceptionModel::ExceptionState::kExceptionRecoverable);
  DCHECK(tester.GetRunLoopCounters().recovered_starts == 2);
}


// ---------------------

void test_co_async_capsule_unrecov_runphase(TestInfo& ti) {
  DefaultCapsuleObjectTester tester(ti, [](ThreadModel& tm) {
    auto obj = make_unique<MyInitiator>(tm);
    obj->EnableUnrecovExceptionInjectionRunPhase();
    return obj;
    });
  tester.ExecuteExpect(RunLoop::LoopExitCause::kUnrecoverableExceptionCause,
    ExceptionModel::ExceptionState::kExceptionUnrecoverable);
}

void test_co_async_capsule_recov_runphase(TestInfo& ti) {
  DefaultCapsuleObjectTester tester(ti, [](ThreadModel& tm) {
    auto obj = make_unique<MyInitiator>(tm);
    obj->EnableRecovExceptionInjectionRunPhase();
    return obj;
    },
    2, 2);
  tester.ExecuteExpect(RunLoop::LoopExitCause::kIterLimitReachedCause,
    ExceptionModel::ExceptionState::kExceptionRecoverable);
  DCHECK(tester.GetRunLoopCounters().recovered_runs == 2);
}


// ---------------------

void test_co_async_capsule_unhandled_excp(TestInfo& ti) {
  DefaultCapsuleObjectTester tester(ti, [](ThreadModel& tm) {
    auto obj = make_unique<MyInitiator>(tm);
    obj->EnableInvalidArgumentInjection();
    return obj;
    });
  
  bool caught = false;

  try {
    RunLoop::LoopExitCause exit_cause;
    ExceptionModel::ExceptionState es;

    tester.Execute(exit_cause, es);
    NOTREACHED();
  }
  catch (std::invalid_argument& e) {
    syslog(_INFO) << e.what() << "\n";
    caught = true;
  }
  DCHECK(caught);
}







