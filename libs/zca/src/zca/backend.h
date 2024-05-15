#pragma once

#include "zca/core/back/backend_builder.h"

class Backend
  :
  public CoreFacade<core::back::BackendCore, core::back::BackendBuildTypes, core::back::BackendBuilder,
  BackendConfig, BackendSeparationConfig>
{
public:
  using CoreFacade::CoreFacade;

  virtual ~Backend() = default;
};


