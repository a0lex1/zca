#include "./loop_object_set.h"

#include "co/xlog/xlog.h"

#define llogd() Log(_DBG) << "LoopObjectSet " << SPTR(this) << " "

namespace co {
namespace async {

DEFINE_XLOGGER_SINK("loopobjset", gCoAsyncLoopObjectSetSink);
#define XLOG_CURRENT_SINK gCoAsyncLoopObjectSetSink

LoopObjectSet::LoopObjectSet(Shptr<Strand> strand) :
  Fibered(strand),
  tss_impl_(*this, Fibered::GetFiberStrandShptr())
{

}

Shptr<Strand> LoopObjectSet::GetStrandShptr() {
  return Fibered::GetFiberStrandShptr();
}

LoopObjectSet& LoopObjectSet::AddObject(Shptr<LoopObject> lobj)
{
  objects_.emplace_back(lobj);
  return *this;
}

bool LoopObjectSet::HaveUnresetableObjects() const
{
  for (auto& obj : objects_) {
    if (!obj->IsResetSupported()) {
      return true;
    }
  }
  return false;
}

void LoopObjectSet::PrepareToStart(Errcode& err)
{
  size_t n = 0;
  for (auto& obj : objects_) {
    obj->PrepareToStart(err);
    if (err) {
      llogd() << "object#" << n << " failed to PrepareToStart (err " << err << ")\n";
    }
    n += 1;
  }
}

void LoopObjectSet::CleanupAbortedStop()
{
  for (auto& obj : objects_) {
    obj->CleanupAbortedStop();
  }
}

bool LoopObjectSet::IsResetSupported() const
{
  // We support it, but other objects may not
  return !(HaveUnresetableObjects());
}

void LoopObjectSet::ResetToNextRun()
{
  objects_.clear();
  for (auto& obj : objects_) {
    obj->ResetToNextRun();
  }
}

void LoopObjectSet::StartObjects(RefTracker rt) {
  for (size_t slot = 0; slot < objects_.size(); slot++) {
    RefTracker rto(CUR_LOC(), [=]() {
      llogd() << "object#" << slot << " #ioended (objs.size()=" << objects_.size() << ")\n";
                   }, rt);
    objects_[slot]->Start(rto);
  }
}

void LoopObjectSet::Start(RefTracker rt)
{
  DCHECK(!started_);

  tss_impl_.BeforeStartWithIoEnded(rt, rt);

  started_ = true;

  StartObjects(RefTracker(CUR_LOC(), [&]() {
    // Reserved. For nothing.
                        }, rt));
}

void LoopObjectSet::StopThreadsafe()
{
  tss_impl_.StopThreadsafe();
}

void LoopObjectSet::StopUnsafe()
{
  // inside our fiber
  // inside our fiber
  for (auto& object : objects_) {
    object->StopThreadsafe();
  }
}

}}



