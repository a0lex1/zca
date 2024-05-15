#pragma once

#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace co {

class TimeSnapshot {
public:
  TimeSnapshot() {
    SnapshotNow();
  }
  void SnapshotNow() {
    last_snapshot_ = boost::posix_time::microsec_clock ::local_time();
  }
  boost::posix_time::time_duration Elapsed() const {
    return boost::posix_time::microsec_clock::local_time() - last_snapshot_;
  }
private:
  boost::posix_time::ptime last_snapshot_;
};


}
