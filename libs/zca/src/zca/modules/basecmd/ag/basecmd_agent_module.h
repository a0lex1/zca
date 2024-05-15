#pragma once

#include "zca/modules/basecmd/basecmd_module.h"
#include "zca/core/ag/agent_api.h"
#include "zca/engine/sdk/module.h"

namespace modules {
namespace basecmd {
namespace ag {

class BasecmdAgentModule : public BasecmdModule, private core::ag::AgentModuleApi {
public:
  virtual ~BasecmdAgentModule() {}

  BasecmdAgentModule();

private:
  // [Module impl]
  engine::ModuleApi& GetModuleApi() override {
    return *this;
  }
  void OnSessionCreated(engine::EngineSessionApi& sess_api,
                        Uptr<engine::SessionModuleData>& module_data) override;

  void DispatchShellCmd(
    engine::DispatchContext& disp_context,
    engine::DispatchCmdData& disp_data,
    Shptr<Strand>,
    Shptr<engine::DispatchCmdTask>& new_task);

  void DispatchSuicideCmd(
    engine::DispatchContext&,
    engine::DispatchCmdData&,
    Shptr<Strand>,
    Shptr<engine::DispatchCmdTask>& new_task);
};

}}}

