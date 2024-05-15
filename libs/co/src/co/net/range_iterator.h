#pragma once

#include "co/net/distance.h"
#include "co/net/address_range.h"
#include "co/common.h"

#include <algorithm>

namespace co {
namespace net {

class range_iterator:
  public std::iterator<
  std::random_access_iterator_tag, boost::asio::ip::address>
{
  friend class address_range;

public:
  typedef boost::asio::ip::address address;
  typedef boost::asio::ip::address_v4 address_v4;
  typedef boost::asio::ip::address_v6 address_v6;

  range_iterator() { }
  range_iterator(const address& v): v_(v) { }
  bool operator!=(range_iterator const& r) const { return !operator==(r); }
  bool operator==(range_iterator const& r) const { return v_ == r.v_; }
  range_iterator::reference operator*() const {
    return const_cast<range_iterator::reference>(v_);
  }
  range_iterator& operator++() {
    if (v_.is_v4()) {
      v_ = address_v4(v_.to_v4().to_ulong() + 1);
    }
    else {
      DCHECK(v_.is_v6());
      DCHECK(!"NOT IMPLEMENTED");
    }
    return *this;
  }
  range_iterator operator+(const distance& d) const {
    DCHECK (v_.is_v4() == d.can_be_dword());
    address n(v_ + d);
    return range_iterator(n);
  }
  range_iterator operator-(const distance& d) const {
    DCHECK (v_.is_v4() == d.can_be_dword());
    address n(v_ - d);
    return range_iterator(n);
  }
  distance operator-(const range_iterator& it) const {
    DCHECK (it.v_.is_v4() == v_.is_v4());
    return distance(v_ - *it);
  }

private:
  address v_;
};

//////////////////////////////////////////////////////////////////////////

static range_iterator range_begin(const address_range& v) {
  return range_iterator(v.first_ip());
}
static range_iterator range_end(const address_range& v) {
  return ++range_iterator(v.last_ip());
}

}}
