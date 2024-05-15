#include "zca/core/back/backend_bot_user_data.h"

namespace core {
namespace back {

void BackendBotUserData::SetBotModuleData(size_t module_index, Uptr<BackendBotModuleData> module_data)
{
  DCHECK(module_index < data_slots_.size());
  data_slots_[module_index] = std::move(module_data);
}

BackendBotModuleData* BackendBotUserData::GetBotModuleData(size_t module_index)
{
  DCHECK(module_index < data_slots_.size());
  return data_slots_[module_index].get();
}

}}
