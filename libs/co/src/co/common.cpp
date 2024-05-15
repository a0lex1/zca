#include "co/common.h"

#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>

namespace co {

void SleepMsec(uint32_t msec) {
  boost::this_thread::sleep(boost::posix_time::milliseconds(msec));
}


}


