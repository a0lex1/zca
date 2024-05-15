#pragma once

#include "co/async/capsule/loop_options.h"
#include "co/async/capsule/cooldowns.h"
#include "co/async/capsule/failsafe_model.h"
#include "co/async/capsule/restarter.h"
#include "co/async/loop_object.h"
#include "co/async/thread_model.h"

namespace co {
namespace async {
namespace capsule {

class CapsuleObjectFactory {
public:
  virtual ~CapsuleObjectFactory() = default;

  // NOTE: CreateIoInitiator is not wrapped in try/catch.
  // Details: It is called first time from ExceptionModel's
  // DoWrappedInitialize, and then by restarter without try/catch at all

  virtual Uptr<Restarter> CreateRestarter() = 0;
  virtual Uptr<ThreadModel> CreateThreadModel() = 0;
  virtual Shptr<LoopObject> CreateIoInitiator(ThreadModel& tm) = 0;
  virtual Uptr<FailsafeModel> CreateFailsafeModel() = 0;
  virtual Uptr<Cooldowns> CreateCooldowns() = 0;
  virtual Uptr<LoopOptions> CreateLoopOptions() = 0;
};

class BasicCapsuleObjectFactory : public CapsuleObjectFactory {
public:
  virtual ~BasicCapsuleObjectFactory() = default;

  BasicCapsuleObjectFactory(bool stop_on_exception = true) : stop_on_excp_(stop_on_exception) {}

  Uptr<Restarter> CreateRestarter() override {
    return std::make_unique<DefaultRestarter>(*this);
  }
  Uptr<ThreadModel> CreateThreadModel() override {
    return std::make_unique<ThreadModel>();
  }
  Shptr<LoopObject> CreateIoInitiator(ThreadModel&) override {
    return std::make_unique<EmptyLoopObject>();
  }
  Uptr<FailsafeModel> CreateFailsafeModel() override {
    return std::make_unique<BasicFailsafeModel>(stop_on_excp_);
  }
  Uptr<Cooldowns> CreateCooldowns() override {
    return std::make_unique<ConservativeCooldowns>();
  }
  Uptr<LoopOptions> CreateLoopOptions() override {
    return std::make_unique<DefaultLoopOptions>();
  }

private:
  bool stop_on_excp_;
};

}}}


