#pragma once

#include "zca/core/front/frontend_core.h"
#include "zca/frontend_config.h"

namespace core {
namespace front {

class FrontendBuilder : public Builder<core::front::FrontendBuildTypes, FrontendConfig, FrontendSeparationConfig> {
public:
  virtual ~FrontendBuilder() = default;

  using TSepObjects = typename core::front::FrontendBuildTypes::SeparatableObjects;
  using TObjects = typename core::front::FrontendBuildTypes::Objects ;
  using TParams = typename core::front::FrontendBuildTypes::Params ;
  using ThreadModel = co::async::ThreadModel;

  Uptr<TSepObjects> BuildSeparatableObjects(ThreadModel& tm, const FrontendConfig&,
    const FrontendSeparationConfig&) override;

  Uptr<TObjects> BuildObjects(ThreadModel& tm, const FrontendConfig&) override;

  Uptr<TParams> BuildParams(const FrontendConfig& conf) override;
};

}}

