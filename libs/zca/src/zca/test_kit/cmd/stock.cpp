#include "zca/test_kit/cmd/stock.h"

#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::posix_time;

namespace stock {

// declared in .h as `extern`
std::vector<CmdexecTestObjectParams> gCmdexecTestParamsVector = {
    // [0]
    CmdexecTestObjectParams(
    cc::BotId::FromUint(0),
    "cmd-exec 00000000000000000000000000000000 -- echo-args 1",
    checkerForCmdScheduled, checkerForCcErrstackSuccess, milliseconds(0)), // no delay_before_stop

    // [1]
    CmdexecTestObjectParams(
      cc::BotId::FromUint(0),
      "cmd-exec 00000000000000000000000000000000 -- echo-args 1",
      checkerForCmdScheduled, checkerForCcErrstackSuccess), // no delay_before_stop
    
    CmdexecTestObjectParams(
      cc::BotId::FromUint(0),
      "cmd-exec 00000000000000000000000000000000 -- echo-args 1",
      checkerForCmdScheduled, checkerForCcErrstackSuccess, milliseconds(10)),

    CmdexecTestObjectParams(
      cc::BotId::FromUint(0),
      "cmd-exec 00000000000000000000000000000000 -- echo-args 1",
      checkerForCmdScheduled, checkerForCcErrstackSuccess, milliseconds(77)),

    CmdexecTestObjectParams(
      cc::BotId::FromUint(0),
      "cmd-exec 00000000000000000000000000000000 -- echo-args 1",
      checkerForCmdScheduled, checkerForCcErrstackSuccess, milliseconds(153)),

    // [5]
    // suicide - sometimes this gives us StreamChunkReaderError in cc err stack, so don't
    // force error checking, use EmptyCheckerForCcErrstack
    CmdexecTestObjectParams(
      cc::BotId::FromUint(0),
      "cmd-exec 00000000000000000000000000000000 -- suicide",
      checkerForCmdScheduled, EmptyCheckerForCcErrstack, milliseconds(153))
  };

}


