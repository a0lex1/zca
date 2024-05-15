#pragma once

#include "co/common.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

namespace co {
namespace async {

class EventBase {
public:
  virtual ~EventBase() = default;

  EventBase(io_context& ioc);

  virtual void SetEvent() = 0;

protected:
  Strand& GetStrand();

private:
  Strand strand_;
};

class Event : public EventBase {
public:
  virtual ~Event() = default;

  using EventBase::EventBase;

  io_context& GetIoctx();

  void AsyncWait(EmptyHandler handler);

  // [EventBase impl]
  void SetEvent() override;

private:
  EmptyHandler cur_wait_handler_;
  bool event_is_set_{false};
};

}}

