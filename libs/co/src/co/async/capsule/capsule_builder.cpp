#include "co/async/capsule/capsule_builder.h"

namespace co {
namespace async {
namespace capsule {

CapsuleBuilder::CapsuleBuilder()
{

}

CapsuleBuilder& CapsuleBuilder::SetRestarterCreator(Func<Uptr<Restarter>()> fn_rlfac)
{
  fn_rlfac_ = fn_rlfac;
  return *this;
}

CapsuleBuilder& CapsuleBuilder::SetThreadModelCreator(Func<Uptr<ThreadModel>()> fn_tmfac)
{
  fn_tmfac_ = fn_tmfac;
  return *this;
}

CapsuleBuilder& CapsuleBuilder::SetIoInitiatorCreator(Func<Shptr<LoopObject>(ThreadModel&)> fn_iofac)
{
  fn_iofac_ = fn_iofac;
  return *this;
}

CapsuleBuilder& CapsuleBuilder::SetFailsafeModelCreator(Func<Uptr<FailsafeModel>()> fn_fsfac)
{
  fn_fsfac_ = fn_fsfac;
  return *this;
}

CapsuleBuilder& CapsuleBuilder::SetCooldownsCreator(Func<Uptr<Cooldowns>()> fn_cdfac)
{
  fn_cdfac_ = fn_cdfac;
  return *this;
}

CapsuleBuilder& CapsuleBuilder::SetLoopOptionsCreator(Func<Uptr<LoopOptions>()> fn_lofac)
{
  fn_lofac_ = fn_lofac;
  return *this;
}

// ---

CapsuleObjectFactory& CapsuleBuilder::GetObjectFactory()
{
  return *this;
}

// ---

Uptr<Restarter> CapsuleBuilder::CreateRestarter()
{
  if (fn_rlfac_) {
    return fn_rlfac_();
  }
  else {
    return BasicCapsuleObjectFactory::CreateRestarter();
  }
}

Uptr<co::async::ThreadModel> CapsuleBuilder::CreateThreadModel()
{
  if (fn_tmfac_) {
    return fn_tmfac_();
  }
  else {
    return BasicCapsuleObjectFactory::CreateThreadModel();
  }

}

Shptr<LoopObject> CapsuleBuilder::CreateIoInitiator(ThreadModel& tm)
{
  if (fn_iofac_) {
    return fn_iofac_(tm);
  }
  else {
    return BasicCapsuleObjectFactory::CreateIoInitiator(tm);
  }

}

Uptr<FailsafeModel> CapsuleBuilder::CreateFailsafeModel()
{
  if (fn_fsfac_) {
    return fn_fsfac_();
  }
  else {
    return BasicCapsuleObjectFactory::CreateFailsafeModel();
  }

}

Uptr<Cooldowns> CapsuleBuilder::CreateCooldowns()
{
  if (fn_cdfac_) {
    return fn_cdfac_();
  }
  else {
    return BasicCapsuleObjectFactory::CreateCooldowns();
  }

}

Uptr<LoopOptions> CapsuleBuilder::CreateLoopOptions()
{
  if (fn_lofac_) {
    return fn_lofac_();
  }
  else {
    return BasicCapsuleObjectFactory::CreateLoopOptions();
  }

}


}}}


