#pragma once

#include "zca/core/back/backend_bot_module_data.h"
#include "cc/cc_user_data.h"
#include "co/common.h"

namespace core {
namespace back {

class BackendBotModuleData;

// CcUserData impl that is attached to cc bot
class BackendBotUserData : public cc::CcUserData {
public:
  virtual ~BackendBotUserData() = default;

  BackendBotUserData(size_t num_modules): data_slots_(num_modules)
  {
  }

  // Not threadsafe. We use this class in BackendCore::AllocBotModuleDataSlots
  // See this code. We don't need to be threadsafe.
  void SetBotModuleData(size_t module_index, Uptr<BackendBotModuleData> module_data);

  // can be nullptr if module doesn't have bot data
  BackendBotModuleData* GetBotModuleData(size_t module_index);

private:
  std::vector<Uptr<BackendBotModuleData>> data_slots_;
};

}}

