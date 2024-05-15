#include "co/async/loop_object_park.h"
#include "co/async/wrap_post.h"

#include "co/xlog/xlog.h"

#define llogd() Log(_DBG) << "LoopObjectPark " << SPTR(this) << " "
#define llogt() Log(_TRACE) << "LoopObjectPark " << SPTR(this) << " "

namespace co {
namespace async {

DEFINE_XLOGGER_SINK("loopobjpark", gCoAsyncLoopObjectParkSink);
#define XLOG_CURRENT_SINK gCoAsyncLoopObjectParkSink

LoopObjectPark::LoopObjectPark(size_t num_clients, bool restart, Shptr<Strand> strand, LoopObjectFactoryFn obj_fac_func) :
  Fibered(strand),
  tss_impl_(*this, GetFiberStrandShptr()),
  num_clients_(num_clients),
  restart_(restart),
  obj_fac_func_(obj_fac_func)
{

}

bool LoopObjectPark::HaveUnresetableObjects() const
{
  for (auto& obj : objects_) {
    if (!obj->IsResetSupported()) {
      return true;
    }
  }
  return false;
}

void LoopObjectPark::PrepareToStart(Errcode& err)
{
  DCHECK(!objects_.size());
  CreateHappyObjects();
  size_t n = 0;
  for (auto& obj : objects_) {
    obj->PrepareToStart(err);
    if (err) {
      llogd() << "LoopObject#" << n << " can't PrepareToStart (err=" << err << ")\n";
      return;
    }
    n += 1;
  }
}

void LoopObjectPark::CleanupAbortedStop() {
  for (auto& obj : objects_) {
    obj->CleanupAbortedStop();
  }
}

void LoopObjectPark::ResetToNextRun()
{
  DCHECK(IsResetSupported());
  objects_.clear();
  for (auto& obj : objects_) {
    obj->ResetToNextRun();
  }
}

void LoopObjectPark::StartHappyObjects(RefTracker rt_all_objs_stopped) {
  // inside unknown Start() context
  // Not synchronized!
  for (size_t slot = 0; slot < num_clients_; slot++) {
    Shptr<LoopObject> obj(objects_[slot]);
    RefTracker rt_obj_stopped(CUR_LOC(),
                              rt_all_objs_stopped.GetContextHandle(),
                              wrap_post(tss_impl_.GetStrand(), co::bind(&LoopObjectPark::OnObjectIoEndedUnsafe,
                                       this,
                                       slot,
                                       rt_all_objs_stopped)));

    obj->Start(rt_obj_stopped);
  }
}

void LoopObjectPark::Start(RefTracker rt)
{
  tss_impl_.BeforeStartWithIoEnded(rt, rt);

  RefTracker rt_objects(CUR_LOC(),
                        // not protected by strand
                        [&] () {
    llogd() << "; rt_objects\n";
  }, rt);

  StartHappyObjects(rt);
}

void LoopObjectPark::StopUnsafe()
{
  DisableRestarting();
  StopHappyObjects();
}

void LoopObjectPark::StopThreadsafe()
{
  tss_impl_.StopThreadsafe();
}

void LoopObjectPark::CreateHappyObjects()
{
  for (size_t slot = 0; slot < num_clients_; slot++) {
    objects_.push_back(obj_fac_func_(slot));
  }
}

void LoopObjectPark::StopHappyObjects()
{
  // inside our fiber
  for (size_t slot = 0; slot < num_clients_; slot++) {
    if (objects_[slot] != nullptr) {
      objects_[slot]->StopThreadsafe();
    }
  }
}

void LoopObjectPark::OnObjectIoEndedUnsafe(size_t slot, RefTracker rt_all_objs_stopped)
{
  DCHECK(IsInsideStrand());

  // inside our fiber
  objects_[slot] = nullptr;
  if (!restart_.load()) {
    return;
  }
  RefTracker rt_obj_stopped(CUR_LOC(), rt_all_objs_stopped.GetContextHandle(),
    wrap_post(tss_impl_.GetStrand(), co::bind(
              &LoopObjectPark::OnObjectIoEndedUnsafe, this, slot, rt_all_objs_stopped)));

  objects_[slot] = obj_fac_func_(slot);
  objects_[slot]->Start(rt_obj_stopped);
}

void LoopObjectPark::DisableRestarting()
{
  restart_.store(false);
}

}}
