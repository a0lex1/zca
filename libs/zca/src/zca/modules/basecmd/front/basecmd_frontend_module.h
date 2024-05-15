#pragma once

#include "zca/modules/basecmd/basecmd_module.h"
#include "zca/core/front/frontend_api.h"
#include "zca/engine/sdk/module.h"

namespace modules {
namespace basecmd {
namespace front {
  
class BasecmdFrontendModule : public BasecmdModule, private core::front::FrontendModuleApi {
public:
  virtual ~BasecmdFrontendModule() = default;

  BasecmdFrontendModule();

private:
  // [Module impl]
  ModuleApi& GetModuleApi() override {
    return *this;
  }
  void OnSessionCreated(engine::EngineSessionApi& sess_api,
    Uptr<engine::SessionModuleData>& module_data) override;

  // [FrontendModuleApi impl]

private:
  void DispatchBotlistCmd(engine::DispatchContext&,
    engine::DispatchCmdData&,
    Shptr<Strand> strand,
    Shptr<engine::DispatchCmdTask>&);

  void DispatchCmdexecCmd(engine::DispatchContext&,
    engine::DispatchCmdData&,
    Shptr<Strand> strand,
    Shptr<engine::DispatchCmdTask>&);

  void DispatchKillCmd(engine::DispatchContext&,
    engine::DispatchCmdData&,
    Shptr<Strand> strand,
    Shptr<engine::DispatchCmdTask>&);

  //void DispatchSyncCmd(engine::DispatchContext&,
  //  engine::DispatchCmdData&,
  //  Shptr<Strand> strand,
  //  Shptr<engine::DispatchCmdTask>&);

  void DispatchExitCmd(engine::DispatchContext&,
    engine::DispatchCmdData&,
    Shptr<Strand> strand,
    Shptr<engine::DispatchCmdTask>&);
};


}}}



