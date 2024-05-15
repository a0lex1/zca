#pragma once

#include "zca/core/back/backend_core.h"

namespace core {
namespace back {

class BackendBuilder : public Builder<core::back::BackendBuildTypes, BackendConfig, BackendSeparationConfig> {
public:
  virtual ~BackendBuilder() = default;

  using TSepObjects = typename core::back::BackendBuildTypes::SeparatableObjects ;
  using TObjects = typename core::back::BackendBuildTypes::Objects ;
  using TParams = core::back::BackendBuildTypes::Params ;
  using ThreadModel = co::async::ThreadModel;

  Uptr<TSepObjects> BuildSeparatableObjects(ThreadModel& tm, const BackendConfig&,
    const BackendSeparationConfig&) override;

  Uptr<TObjects> BuildObjects(ThreadModel& tm, const BackendConfig&) override;

  Uptr<TParams> BuildParams(const BackendConfig& conf) override;
};

}}


