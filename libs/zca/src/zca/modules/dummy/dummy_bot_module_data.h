#pragma once

#include "zca/core/back/backend_api.h"

namespace modules {
namespace dummy {

class DummyBotModuleData : public core::back::BackendBotModuleData {
public:
  virtual ~DummyBotModuleData() = default;

  void SetPenLength(int inches) { pen_len_ = inches; }
  int GetPenLength() const { return pen_len_; }

  void SetMedDosage(int med_dosage) { med_dosage_ = med_dosage; }
  int GetMedDosage() const { return med_dosage_; }

private:
  int pen_len_{ 0 };
  int med_dosage_{ 0 };
};

}}

