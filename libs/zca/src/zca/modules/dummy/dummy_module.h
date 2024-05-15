#pragma once

#include "zca/modules/dummy/dummy_bot_module_data.h"

#include "zca/core/back/backend_api.h"
#include "zca/core/ag/agent_api.h"

#include "zca/engine/sdk/module.h"

namespace modules {
namespace dummy {

class DummyModule : public engine::Module {
public:
  virtual ~DummyModule();

  DummyModule(const char* debug_tag = "");

private:
  using DispatchContext = engine::DispatchContext;
  using DispatchCmdData = engine::DispatchCmdData;
  using DispatchCmdTask = engine::DispatchCmdTask;
  using ModuleApi = engine::ModuleApi;
  using EngineSessionApi = engine::EngineSessionApi;
  using DispatchTable = engine::DispatchTable;
  using SessionModuleData = engine::SessionModuleData;

  // [BackendModule::Module impl]
  void DispatchEchoargsCmd(DispatchContext&, DispatchCmdData&, Shptr<Strand> strand, Shptr<DispatchCmdTask>& new_task);
  void DispatchEchoinputCmd(DispatchContext&, DispatchCmdData&, Shptr<Strand> strand, Shptr<DispatchCmdTask>& new_task);
  void DispatchWaitCmd(DispatchContext&, DispatchCmdData&, Shptr<Strand> strand, Shptr<DispatchCmdTask>& new_task);
  void DispatchSetSessionTagCmd(DispatchContext&, DispatchCmdData&, Shptr<Strand> strand, Shptr<DispatchCmdTask>& new_task);
  void DispatchGetSessionTagCmd(DispatchContext&, DispatchCmdData&, Shptr<Strand> strand, Shptr<DispatchCmdTask>& new_task);
  void DispatchExecCmd(DispatchContext&, DispatchCmdData&, Shptr<Strand> strand, Shptr<DispatchCmdTask>& new_task);

  void OnSessionCreated(EngineSessionApi& sess_api,
                        Uptr<SessionModuleData>& module_data) override;

  const engine::DispatchCmdFuncMap& GetDispatchCmdFuncMap() override;

  ModuleApi& GetModuleApi() override { return *module_api_; }

protected:
  void SetModuleApi(ModuleApi& module_api) {
    module_api_ = &module_api;
  }

private:
  engine::DispatchTable disp_table_;
  ModuleApi* module_api_{ nullptr };
};

class DummyBackendModuleApi:
  public core::back::BackendModuleApi, private core::back::BotPropertyGroup {
public:
  virtual ~DummyBackendModuleApi() = default;

  DummyBackendModuleApi(engine::Module& owner_module)
    :
    owner_module_(owner_module)
  {
  }

private:
  // [BackendModuleApi ]
  void RegisterBotProperties(core::back::BotPropertyGroupRegistrator& reg) override {
    reg.RegisterBotPropertyGroup(*this);
  }

  Uptr<core::back::BackendBotModuleData> CreateBotModuleData() override {
    auto new_data(make_unique<DummyBotModuleData>());
    new_data->SetPenLength(17);
    new_data->SetMedDosage(200);
    return std::move(new_data);
  }

private:
  // [BotPropertyGroup impl]
  const char* GetGroupName() const override {
    return "tmi";
  }

  size_t GetPropertyCount() const override {
    return 2;
  }

  void GetPropertyNames(StringVector& prop_names) const override {
    prop_names.clear();
    prop_names.emplace_back("pen_len");
    prop_names.emplace_back("med_dose");
  }


  void ReadForBot(Shptr<cc::ICcBot> bot,
                  StringVector& props,
                  size_t max_line_length,
                  size_t max_line_count
                  ) override {
    // Let's have some fun.
    props.clear();

    DummyBotModuleData* our_module_data =
      owner_module_.GetLocalApiAs<core::back::BackendLocalApi>()
        .GetBotModuleDataAs<DummyBotModuleData>(*bot.get());

    DCHECK(our_module_data);

    props.push_back(co::string_printf("%d", our_module_data->GetPenLength()));
    props.push_back(co::string_printf("%d", our_module_data->GetMedDosage()));
  }

private:
  engine::Module& owner_module_;
};

class DummyAgentModuleApi: public core::ag::AgentModuleApi {
public:
  virtual ~DummyAgentModuleApi() = default;

  DummyAgentModuleApi(engine::Module& owner_module) : owner_module_(owner_module) {}

private:
  engine::Module& owner_module_;
};

class DummyBackendModule: public DummyModule {
public:
  virtual ~DummyBackendModule() = default;

  DummyBackendModule(): api_(*this) {
    SET_DEBUG_TAG(*this, "DummyBackendModule");
    SetModuleApi(api_);
  }

private:
  DummyBackendModuleApi api_;
};

class DummyAgentModule: public DummyModule {
public:
  virtual ~DummyAgentModule() = default;

  DummyAgentModule() : api_(*this) {
    SET_DEBUG_TAG(*this, "DummyAgentModule");
    SetModuleApi(api_);
  }

private:
  DummyAgentModuleApi api_;
};

class DummyFrontendModule : public DummyModule {
public:
  virtual ~DummyFrontendModule() = default;

  DummyFrontendModule()
  {
  }
};

}}





