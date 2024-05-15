#pragma once

#include "zca/core/ag/agent_core.h"
#include "zca/agent_config.h"

namespace core {
namespace ag {

class AgentBuilder : public Builder<core::ag::AgentBuildTypes, AgentConfig, AgentSeparationConfig> {
public:
  virtual ~AgentBuilder() = default;

  using TSepObjects = typename core::ag::AgentBuildTypes::SeparatableObjects;
  using TObjects = typename core::ag::AgentBuildTypes::Objects ;
  using TParams = typename core::ag::AgentBuildTypes::Params ;
  using ThreadModel = co::async::ThreadModel;

  Uptr<TSepObjects> BuildSeparatableObjects(ThreadModel& tm, const AgentConfig&,
    const AgentSeparationConfig&) override;

  Uptr<TObjects> BuildObjects(ThreadModel& tm, const AgentConfig&) override;

  Uptr<TParams> BuildParams(const AgentConfig& conf) override;
};

}}

