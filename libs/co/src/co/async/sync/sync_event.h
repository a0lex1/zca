#pragma once

#include "co/async/event.h"
#include "co/base/async_coro.h"

namespace co {
namespace async {
namespace sync {

class SyncEvent : public EventBase, public AsyncCoroAdaptor<Event> {
public:
  virtual ~SyncEvent() = default;
  
  SyncEvent(Uptr<Event> adapted_uptr, AsyncCoro& coro)
    :
    EventBase(adapted_uptr->GetIoctx()),
    AsyncCoroAdaptor<Event>(std::move(adapted_uptr), coro)
  {
  }
  SyncEvent(Event& adapted_ref, AsyncCoro& coro)
    :
    EventBase(adapted_ref.GetIoctx()),
    AsyncCoroAdaptor<Event>(adapted_ref, coro)
  {
  }
    
  void Wait() {
    auto cbk(GetCoro().CreateContinuationCallback<>());
    GetCoro().DoYield([&, cbk]() {
      // initiate after returning from coro (before yield)
      GetAdaptedObject().AsyncWait(cbk.GetFunction());
                      });
  }
  void SetEvent() override {
    GetAdaptedObject().SetEvent();
  }
};

}}}

