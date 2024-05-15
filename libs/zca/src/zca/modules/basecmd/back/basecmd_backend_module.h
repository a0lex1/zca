#pragma once

#include "zca/modules/basecmd/basecmd_module.h"
#include "zca/modules/basecmd/back/basecmd_bot_module_data.h"
#include "zca/modules/basecmd/back/cmdinfo_property_group.h"
#include "zca/core/back/backend_api.h"
#include "zca/engine/sdk/module.h"

namespace modules {
namespace basecmd {
namespace back {
  
class BasecmdBackendModule : public BasecmdModule, private core::back::BackendModuleApi {
public:
  virtual ~BasecmdBackendModule() = default;

  BasecmdBackendModule();

private:
  // [Module impl]
  ModuleApi& GetModuleApi() override {
    return *this;
  }
  void OnSessionCreated(engine::EngineSessionApi& sess_api,
    Uptr<engine::SessionModuleData>& module_data) override;

  // [BackendModuleApi impl]
  void RegisterBotProperties(core::back::BotPropertyGroupRegistrator&) override;
  Uptr<core::back::BackendBotModuleData> CreateBotModuleData() override;
  void OnBotHandshake(Shptr<cc::ICcBot>) override;

private:
  void DispatchBotcountCmd(engine::DispatchContext&,
    engine::DispatchCmdData&,
    Shptr<Strand>,
    Shptr<engine::DispatchCmdTask>&);

  void DispatchBotlistCmd(engine::DispatchContext&,
    engine::DispatchCmdData&,
    Shptr<Strand>,
    Shptr<engine::DispatchCmdTask>&);

  void DispatchCmdexecCmd(engine::DispatchContext&,
    engine::DispatchCmdData&,
    Shptr<Strand>,
    Shptr<engine::DispatchCmdTask>&);

  void DispatchKillCmd(engine::DispatchContext&,
    engine::DispatchCmdData&,
    Shptr<Strand>,
    Shptr<engine::DispatchCmdTask>&);

  void DispatchExitCmd(engine::DispatchContext&,
    engine::DispatchCmdData&,
    Shptr<Strand>,
    Shptr<engine::DispatchCmdTask>&);

private:
  Uptr<CmdinfoPropertyGroup> cmdinfo_prop_group_;
};


}}}



