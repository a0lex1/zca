#pragma once

#include "zca/core/ag/agent_builder.h"

class Agent
  :
  public CoreFacade<core::ag::AgentCore, core::ag::AgentBuildTypes, core::ag::AgentBuilder,
  AgentConfig, AgentSeparationConfig>
{
public:
  using CoreFacade::CoreFacade;

  virtual ~Agent() = default;
};


