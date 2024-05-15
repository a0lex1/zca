#include "co/async/capsule/restarter.h"
#include "co/async/capsule/capsule_object_factory.h"

namespace co {
namespace async {
namespace capsule {

DefaultRestarter::DefaultRestarter(CapsuleObjectFactory& objfac,
                                         bool recreate_thread_model,
                                         bool force_recreate_io_initiator)
  :
  Restarter(objfac),
  recreate_thread_model_(recreate_thread_model),
  force_recreate_io_initiator_(force_recreate_io_initiator)
{
}

void DefaultRestarter::DoRestartAfterInitException(
  Uptr<ThreadModel>& tm,
  Shptr<LoopObject>& io_initiator)
{
  CommonRestart(tm, io_initiator);
}

void DefaultRestarter::DoRestartAfterRunException(
  Uptr<ThreadModel>& tm,
  Shptr<LoopObject>& io_initiator)
{
  CommonRestart(tm, io_initiator);
}

void DefaultRestarter::DoRestartAfterNormalExit(
  Uptr<ThreadModel>& tm,
  Shptr<LoopObject>& io_initiator)
{
  CommonRestart(tm, io_initiator);
}

void DefaultRestarter::CommonRestart(
  Uptr<ThreadModel>& tm,
  Shptr<LoopObject>& io_initiator)
{
  bool need_recreate_initiator = true;

  if (!recreate_thread_model_) {
    if (io_initiator->IsResetSupported()) {
      if (!force_recreate_io_initiator_) {
        // no need to recreate initiator, it supports reset,
        // tm is not recreated and force flag not specified
        io_initiator->ResetToNextRun();
        need_recreate_initiator = false;
      }
      else {
        // reset supported but recreate is forced
      }
    }
    else {
      // reset not enabled, need recreate
    }
  }
  if (need_recreate_initiator) {
    // 1) delete initiator
    io_initiator = nullptr;
  }
  // 2) recreate/reset thread model
  if (recreate_thread_model_) {
    DCHECK(need_recreate_initiator);
    tm = nullptr;
    tm = GetCapsuleObjectFactory().CreateThreadModel();
  }
  else {
    tm->Restart();
  }
  if (need_recreate_initiator) {
    // 3) create initiator
    io_initiator = GetCapsuleObjectFactory().CreateIoInitiator(*tm.get());
  }
}

}}}



