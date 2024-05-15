#pragma once

#include "co/async/loop_object.h"
#include "co/async/thread_model.h"

#include "co/base/flags.h"

namespace co {
namespace async {
namespace capsule {

class CapsuleObjectFactory;
class Restarter {
public:
  virtual ~Restarter() = default;

  Restarter(CapsuleObjectFactory& objfac) : objfac_(objfac) {}

  //after recoverable exception
  virtual void DoRestartAfterInitException(Uptr<ThreadModel>& tm, // Uptr<>&
                                           Shptr<LoopObject>& io_initiator) = 0; // Uptr<>&

  virtual void DoRestartAfterRunException(Uptr<ThreadModel>& tm, // Uptr<>&
                                          Shptr<LoopObject>& io_initiator) = 0;

  virtual void DoRestartAfterNormalExit(Uptr<ThreadModel>& tm, // Uptr<>&
                                        Shptr<LoopObject>& io_initiator) = 0;

protected:
  CapsuleObjectFactory& GetCapsuleObjectFactory() { return objfac_; }

private:
  CapsuleObjectFactory& objfac_;
};

// ---

class DefaultRestarter : public Restarter {
public:
  virtual ~DefaultRestarter() = default;

  DefaultRestarter(CapsuleObjectFactory& objfac,
                   bool recreate_thread_model = true, //#UafRecreateThreadModel
                   bool force_recreate_io_initiator = false);

  void DoRestartAfterInitException(Uptr<ThreadModel>& tm, // Uptr<>&
                                    Shptr<LoopObject>& io_initiator) override;

  void DoRestartAfterRunException(Uptr<ThreadModel>& tm, // Uptr<>&
                                  Shptr<LoopObject>& io_initiator) override;

  void DoRestartAfterNormalExit(Uptr<ThreadModel>& tm, // Uptr<>&
                                Shptr<LoopObject>& io_initiator) override;

private:
  virtual void CommonRestart(Uptr<ThreadModel>& tm, // Uptr<>&
                             Shptr<LoopObject>& io_initiator);
private:
  bool recreate_thread_model_;
  bool force_recreate_io_initiator_;
};

}}}


