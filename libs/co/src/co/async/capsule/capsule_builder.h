#pragma once

#include "co/async/capsule/capsule.h"

namespace co {
namespace async {
namespace capsule {

class CapsuleBuilder : private BasicCapsuleObjectFactory {
public:
  CapsuleBuilder();

  CapsuleBuilder& SetRestarterCreator(Func<Uptr<Restarter>()> fn_rlfac);
  CapsuleBuilder& SetThreadModelCreator(Func<Uptr<ThreadModel>()> fn_tmfac);
  CapsuleBuilder& SetIoInitiatorCreator(Func<Shptr<LoopObject>(ThreadModel&)> fn_iofac);
  CapsuleBuilder& SetFailsafeModelCreator(Func<Uptr<FailsafeModel>()> fn_fsfac);
  CapsuleBuilder& SetCooldownsCreator(Func<Uptr<Cooldowns>()> fn_cdfac);
  CapsuleBuilder& SetLoopOptionsCreator(Func<Uptr<LoopOptions>()> fn_lofac);

  CapsuleObjectFactory& GetObjectFactory();

private:
  // [CapsuleObjectFactory impl]
  Uptr<Restarter> CreateRestarter() override;
  Uptr<ThreadModel> CreateThreadModel()  override;
  Shptr<LoopObject> CreateIoInitiator(ThreadModel& tm)  override;
  Uptr<FailsafeModel> CreateFailsafeModel()  override;
  Uptr<Cooldowns> CreateCooldowns() override;
  Uptr<LoopOptions> CreateLoopOptions() override;

private:
  Func<Uptr<Restarter>()> fn_rlfac_;
  Func<Uptr<ThreadModel>()> fn_tmfac_;
  Func<Shptr<LoopObject>(ThreadModel&)> fn_iofac_;
  Func<Uptr<FailsafeModel>()> fn_fsfac_;
  Func<Uptr<Cooldowns>()> fn_cdfac_;
  Func<Uptr<LoopOptions>()> fn_lofac_;
};

}}}

