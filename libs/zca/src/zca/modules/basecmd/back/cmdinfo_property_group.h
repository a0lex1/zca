#pragma once

#include "zca/core/back/backend_api.h"

namespace modules {
namespace basecmd {
namespace back {

class BasecmdBackendModule;

// Backend's side of basecmd module adds cmdinfo property group
// that is displayed by bot-list command. This can be accessed by
// other modules through backend api.

class CmdinfoPropertyGroup : public core::back::BotPropertyGroup {
public:
  virtual ~CmdinfoPropertyGroup() = default;

  CmdinfoPropertyGroup(core::back::BackendLocalApi& backend_local_api,
                       core::back::BackendGlobalApi& backend_global_api);

  const char* GetGroupName() const override {

    return "cmdinfo";
  }

  size_t GetPropertyCount() const override {

    return 8;
  }

  void GetPropertyNames(StringVector& prop_names) const override {
    prop_names.clear();
    //prop_names.emplace_back("stcode"); // cmd state code
    prop_names.emplace_back("cmdstate");
    prop_names.emplace_back("cmd");
    prop_names.emplace_back("cmdres");
    prop_names.emplace_back("postcmd");
    prop_names.emplace_back("postres");
    prop_names.emplace_back("neterr");
    prop_names.emplace_back("netshellerr");
    prop_names.emplace_back("salt");
  }

  void ReadForBot(Shptr<cc::ICcBot> bot,
                  StringVector& props,
                  size_t max_line_length,
                  size_t max_line_count) override;

private:
  core::back::BackendLocalApi& backend_local_api_;
  core::back::BackendGlobalApi& backend_global_api_;
};

}}}
