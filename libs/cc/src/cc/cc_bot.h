#pragma once

#include "cc/cc_error.h"
#include "cc/cc_bot_data.h"
#include "cc/cc_user_data.h"

#include "co/async/startable_stopable.h"
#include "co/common.h"

namespace cc {

class ICcBot {
public:
  virtual ~ICcBot() = default;

  virtual const std::vector<CcError>& GetLastErrorVector() const = 0;

  // Threadsafe
  virtual void Kill() = 0;

  // Threadsafe
  virtual ICcBotReadonlyData& GetReadonlyData() = 0;

  // Threadsafe. GetUserData() can return nullptr if SetUserData wasn't yet called
  virtual void SetUserData(BoostShptr<CcUserData> user_data) = 0;
  virtual BoostShptr<CcUserData> GetUserData() = 0;

  // Threadsafe
  virtual void ExecuteSequencedCommand(Uptr<std::string> cmd_opaque_data,
                                       std::string& cmd_result_opaque_data,
                                       HandlerWithErrcode handler) = 0;
};

}





