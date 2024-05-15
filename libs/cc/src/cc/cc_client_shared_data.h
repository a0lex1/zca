#pragma once

#include "cc/cc_error.h"
#include "cc/cc_client_bot_options.h"

#include <vector>

namespace cc {

// CcClient can have Session=nullptr (when the session is ended) AND
// Session can live without CcClient (if we're restarting RunLoop
// with recreate_thread_model=false, e.g. when previous loop's sessions
// are getting destroyed in the next loop)
// This is why we need CcClientSharedData.
struct CcClientSharedData {

  CcClientSharedData(const CcClientBotOptions& bot_opts)
    :
    bot_opts_(bot_opts)
  {
  }

  std::vector<CcError> err_stack_;
  CcClientBotOptions bot_opts_;
  Uptr<boost::posix_time::time_duration> ping_interval_;
};

}
