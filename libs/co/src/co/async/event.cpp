#include "co/async/event.h"

using namespace std;

namespace co {
namespace async {

EventBase::EventBase(io_context& ioc) : strand_(ioc)
{

}

Strand& EventBase::GetStrand()
{
  return strand_;
}

io_context& Event::GetIoctx()
{
  return GetStrand().context();
}

void Event::AsyncWait(EmptyHandler handler)
{
  boost::asio::post(GetStrand(),
    [&, handler]() {
        DCHECK(GetStrand().running_in_this_thread());
        DCHECK(!cur_wait_handler_); // parallel waits not supported
        if (event_is_set_) {
          // event is set now, just call the handler
          handler();
        }
        else {
          // `schedule` the handler
          cur_wait_handler_ = handler;
        }
   });
}

void Event::SetEvent()
{
  boost::asio::post(GetStrand(),
    [&]() {
      DCHECK(GetStrand().running_in_this_thread());
      event_is_set_ = true;
      if (cur_wait_handler_) {
        auto wait_handler_copy(cur_wait_handler_);
        cur_wait_handler_ = nullptr;
        wait_handler_copy();
      }
   });
}

}}
