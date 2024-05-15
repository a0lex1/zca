#pragma once

#include "co/base/cmdline/keyed_cmdline.h"
#include "co/base/redirect_std_handle.h"
#include "co/base/dict.h"
#include <iostream>

static void InitHandlerTrackingIfEnabled(co::cmdline::KeyedCmdLine<char>& cl) {
#ifdef BOOST_ASIO_ENABLE_HANDLER_TRACKING
#ifdef _NDEBUG
#pragma error
#endif
  using namespace std;
  using namespace co;
  bool do_redirect_stderr = false;
  OverrideFromDictNoexcept<string, string, bool>(cl.GetNamedArgs(),
                                              "stderr-to-file",
                                              do_redirect_stderr,
                                              ConsumeAction::kConsume);
  if (do_redirect_stderr) {
    std::cout << "Redirecting stderr to file\n";
    RedirectStderrToFile("std_out_err.txt");
  }
  BOOST_ASIO_HANDLER_TRACKING_INIT; // Deleaker detects memleak here
#endif
}

