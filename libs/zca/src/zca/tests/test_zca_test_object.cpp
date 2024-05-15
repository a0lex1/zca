#include "zca/modules/basecmd/back/basecmd_backend_module.h"
#include "zca/modules/basecmd/ag/basecmd_agent_module.h"
#include "zca/modules/dummy/dummy_module.h"
#include "zca/test_kit/zca_test_object.h"

#include "co/async/test_kit/default_capsule_object_tester.h"
#include "co/base/recoverable_exception.h"
#include "co/base/unrecoverable_exception.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;
using namespace co::async::capsule;
using namespace co::async::test_kit;

namespace {

class MyTestObject : public ZcaTestObject {
public:
  virtual ~MyTestObject() = default;

  using ZcaTestObject::ZcaTestObject;

  void SetEnableUnrecovExceptionInjection() { enable_unrecov_exception_inj_ = true; }
  void SetEnableRecovExceptionInjection() { enable_recov_exception_inj_ = true; }
  void SetEnableModules() { enable_modules_ = true; }

private:
  void EnableParties() override {
    syslog(_INFO) << __FUNCTION__ << "\n";
    EnableBackend();
    EnableAgent();
    EnableBackParaNetshellClient();
  }
  void AddModules() override {
    syslog(_INFO) << __FUNCTION__ << "\n";
    if (enable_modules_) {
      abort(); // TODO
    }
  }
  void SetOptions() override {
    syslog(_INFO) << __FUNCTION__ << "\n";
  }

  void OnTripletStarted() override {
    on_triplet_started_++;
    syslog(_INFO) << __FUNCTION__ << "\n";
  }
  void OnAllConnected(RefTracker rt) override {
    on_all_connected_++;
    DCHECK(!(enable_unrecov_exception_inj_ && enable_recov_exception_inj_)); // not both
    syslog(_INFO) << __FUNCTION__ << "\n";
    if (enable_unrecov_exception_inj_) {
      throw UnrecoverableException();
    }
    if (enable_recov_exception_inj_) {
      throw RecoverableException();
    }
    StopThreadsafe();
  }
private:
  bool enable_unrecov_exception_inj_{ false };
  bool enable_recov_exception_inj_{ false };
  bool enable_modules_{ false };
  unsigned on_triplet_started_{ 0 }, on_shells_connected_{ 0 }, on_all_connected_{ 0 };
};
}


// This test without exception
void test_zca_test_object(TestInfo& ti) {
  DefaultCapsuleObjectTester tester(ti, [](ThreadModel& tm) {
    auto obj = make_unique<MyTestObject>(tm);
    return obj;
    });
  tester.ExecuteExpect(RunLoop::LoopExitCause::kIterLimitReachedCause);
}

// Test with unrecoverable exception
void test_zca_test_object_unrecov_exception(TestInfo& ti) {
  DefaultCapsuleObjectTester tester(ti, [](ThreadModel& tm) {
    auto obj = make_unique<MyTestObject>(tm);
    obj->SetEnableUnrecovExceptionInjection();
    return obj;
    });
  tester.ExecuteExpect(RunLoop::LoopExitCause::kUnrecoverableExceptionCause,
    ExceptionModel::ExceptionState::kExceptionUnrecoverable);
}

// Test with recoverable exception
void test_zca_test_object_recov_exception(TestInfo& ti) {
  DefaultCapsuleObjectTester tester(ti, [](ThreadModel& tm) {
    auto obj = make_unique<MyTestObject>(tm);
    obj->SetEnableRecovExceptionInjection();
    return obj;
    },
    2, 2);
  tester.ExecuteExpect(RunLoop::LoopExitCause::kIterLimitReachedCause,
    ExceptionModel::ExceptionState::kExceptionRecoverable);
  DCHECK(tester.GetRunLoopCounters().recovered_runs == 2);
}


