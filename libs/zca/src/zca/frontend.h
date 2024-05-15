#pragma once

#include "zca/core/front/frontend_builder.h"

class Frontend
  :
  public CoreFacade<core::front::FrontendCore, core::front::FrontendBuildTypes, core::front::FrontendBuilder,
  FrontendConfig, FrontendSeparationConfig>
{
public:
  using CoreFacade::CoreFacade;

  virtual ~Frontend() = default;
};


