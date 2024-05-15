#pragma once

#include "cc/bot_id.h"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace cc {

class CcClientBotOptions {
public:
  using time_duration = boost::posix_time::time_duration;

  CcClientBotOptions(const BotId& bot_id,
                     bool hshake_postpone_enable,
                     time_duration hshake_postpone_delay = time_duration())
    :
    bot_id_(bot_id),
    hshake_postpone_enable_(hshake_postpone_enable),
    hshake_postpone_delay_(hshake_postpone_delay)
  {
  }

  const BotId& GetBotId() const { return bot_id_; }
  bool GetPostponeEnable() const { return hshake_postpone_enable_; }
  time_duration GetPostponeDelay() const { return hshake_postpone_delay_; }

  // For tests
  void _SetInjectHandleWriteCommandResultError(bool enable) {
    _inj_handle_wr_cmdres_err_ = enable;
  }
  bool _IsInjectHandleWriteCommandResultErrorEnabled() const {
    return _inj_handle_wr_cmdres_err_;
  }

private:
  BotId bot_id_;
  bool hshake_postpone_enable_{false};
  time_duration hshake_postpone_delay_;

  // For tests
  bool _inj_handle_wr_cmdres_err_{ false };
};

}

