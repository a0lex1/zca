#pragma once

#include "zca/core/back/backend_bot_user_data.h"
#include "zca/core/back/backend_bot_module_data.h"
#include "zca/engine/sdk/module.h"

#include "cc/cc_bot.h"
#include "cc/cc_server.h"

namespace core {
namespace back {

class BackendCore;
class BackendLocalApi;
class BackendGlobalApi;
class BackendModuleApi;

class BotPropertyGroup {
public:
  virtual ~BotPropertyGroup() = default;

  virtual const char* GetGroupName() const = 0;

  virtual size_t GetPropertyCount() const = 0;

  virtual void GetPropertyNames(StringVector& prop_names) const = 0;

  // Threadsafe
  // You must implement threadsafe here!
  // If you need interlocked, implement your own interlocked object and
  // store it as BotUserData.
  // ReadForBot() reads entire group for some bot.
  virtual void ReadForBot(Shptr<cc::ICcBot> bot,
                          StringVector& props,
                          size_t max_line_length,
                          size_t max_line_count) = 0;
};

class BotPropertyGroupRegistrator;
class BotPropertyGroupSet {
public:
  size_t GetBotPropertyGroupCount() const {
    return groups_.size();
  }

  BotPropertyGroup& GetBotPropertyGroup(size_t group_index) {
    DCHECK(group_index < groups_.size());
    return *groups_[group_index];
  }

private:
  friend class BotPropertyGroupRegistrator;
  auto& GetGroups() { return groups_; }

private:
  std::vector<BotPropertyGroup*> groups_;
};

class BotPropertyGroupRegistrator {
public:
  BotPropertyGroupRegistrator(BotPropertyGroupSet& prop_group_set)
    : prop_group_set_(prop_group_set)
  {
    
  }

  void RegisterBotPropertyGroup(BotPropertyGroup& prop_group)   {
    prop_group_set_.GetGroups().push_back(&prop_group);
  }
private:
  BotPropertyGroupSet& prop_group_set_;
};

// To implement by module, backend uses it
class BackendModuleApi : public engine::ModuleApi {
public:
  virtual ~BackendModuleApi() = default;

  virtual void RegisterBotProperties(BotPropertyGroupRegistrator&) {
    // empty by default
  }

  virtual Uptr<BackendBotModuleData> CreateBotModuleData() {
    // empty by default
    return nullptr;
  }

  virtual void OnBotHandshake(Shptr<cc::ICcBot> bot) {
    // empty by default
  }
};

// Local for each module - side of backend API
class BackendLocalApi : public engine::LocalApi {
public:
  virtual ~BackendLocalApi() = default;

  BackendBotModuleData* GetBotModuleData(cc::ICcBot& bot) {
    BoostShptr<cc::CcUserData> bot_user_data = bot.GetUserData();
    if (!bot_user_data) {
      return nullptr;
    }
    auto our_bot_user_data =
      boost::static_pointer_cast<BackendBotUserData>(bot_user_data);

    // GetBotModuleData doesn't need to be threadsafe. We set it (with ->SetBotModuleData)
    // in BackendCore::AllocBotModuleDataSlots. It's already connected to
    // cc user data when the cc user data is set for bot.
    return our_bot_user_data->GetBotModuleData(GetModule().GetModuleIndex());
  }

  template <typename T>
  T* GetBotModuleDataAs(cc::ICcBot& bot) {
    return static_cast<T*>(GetBotModuleData(bot));
  }

private:
  friend class BackendCore;
  BackendLocalApi() {}
};

// Global for all modules - side of backend API
class BackendGlobalApi : public engine::GlobalApi {
public:
  virtual ~BackendGlobalApi() = default;

  BackendGlobalApi(cc::CcServer& cc_server, BotPropertyGroupSet& bot_propg_set)
    : cc_server_(cc_server), bot_propg_set_(bot_propg_set)
  {
  }

  cc::CcServer& GetCcServer() {
    return cc_server_;
  }

  BotPropertyGroupSet& GetBotPropertyGroupSet() {
    return bot_propg_set_;
  }

private:
  cc::CcServer& cc_server_;
  BotPropertyGroupSet& bot_propg_set_;
};

}}
