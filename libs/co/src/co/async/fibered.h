#pragma once

#include "co/common.h"
#include <boost/asio/strand.hpp>

namespace co {
namespace async {

class Fibered {
public:
  virtual ~Fibered() = default;

  Fibered(Shptr<Strand> strand) : strand_(strand)
  {
  }
  Strand& GetFiberStrand() { return *strand_.get(); }

  Shptr<Strand> GetFiberStrandShptr() { return strand_; }
  bool IsInsideFiberStrand() {
    return strand_->running_in_this_thread();
  }
private:
  Shptr<Strand> strand_;
};

}}


