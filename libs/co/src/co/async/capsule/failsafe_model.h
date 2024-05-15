#pragma once

#include "co/async/capsule/exception_model.h"

namespace co {
namespace async {
namespace capsule {

class FailsafeModel {
public:
  virtual ~FailsafeModel() = default;

  virtual ExceptionModel& GetExceptionModel() = 0;
  //GetAcceptorErrorLogic()
};

class BasicFailsafeModel : public FailsafeModel {
public:
  virtual ~BasicFailsafeModel() = default;

  BasicFailsafeModel(bool em_stop_on_exception)
    : basic_model_(em_stop_on_exception)
  {

  }

  ExceptionModel& GetExceptionModel() override {
    return basic_model_;
  }
private:
  BasicExceptionModel basic_model_;
};

}}}

