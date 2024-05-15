#pragma once

#include "co/async/loop_object_park.h"
#include "co/async/client.h"
#include "co/async/stream_factory.h"
#include "co/xlog/define_logger_sink.h"

namespace co {
namespace async {

// SessionPark is LEGACY for tests, remade with ObjectPark inside
class SessionPark
  : public Startable, public ThreadsafeStopable
{
public:
  virtual ~SessionPark() = default;

  // You must implement a function that creates new sessions
  using client_sess_factory_func = Func<Shptr<Session>(Uptr<Stream>, size_t slot)>;
  using Endpoint = co::net::Endpoint;
  
  SessionPark(size_t num_clients,
              bool restart,
              Endpoint addr,
              client_sess_factory_func fac_func,
              Shptr<StreamFactory> stream_factory,
              Shptr<StreamConnector> connector)
    :
    addr_(addr),
    fac_func_(fac_func),
    stream_factory_(stream_factory),
    connector_(connector),
    object_park_(num_clients, // !can't specify thread separation (strand) for object park
                 restart,
                 make_shared<Strand>(stream_factory_->GetIoContext()),
                 co::bind(&SessionPark::CreateObject, this, _1))
  {

  }

  void Start(RefTracker rt) override {
    // PrepareToStart() can throw meanwhile we're in DoWrappedStart(), not Initialize
    // This looks normal, but this class is trash anyway
    object_park_.PrepareToStartNofail();
    object_park_.Start(rt);
  }

  // Stopping means stop restarting clients. After this, call StopClients() - guaranteed graceful stop
  void StopThreadsafe() override {
    // After this call, RefTrackers can fire in some threads
    object_park_.StopThreadsafe();
  }

private:
  //void StopUnsafe() override { //<<<<<<< TO -> ObjectPark
  //  restart_ = false;
  //}

  // Called by ObjectPark when it's creating/recreating an object in slot
  Shptr<LoopObject> CreateObject(size_t slot) {
    // INSIDE UNKNOWN STRAND (object_park_ responsibility)
    // Call user's fac_func_, it creates the session
    // We're gonna run this session in a newly created client.
    Uptr<Stream> new_stm = stream_factory_->CreateStream();
    Shptr<Client> new_client = make_shared<Client>(
      addr_, connector_,
      fac_func_(std::move(new_stm), slot));
    return new_client;
  }

private:
  Endpoint addr_;
  client_sess_factory_func fac_func_;
  Shptr<StreamFactory> stream_factory_;
  Shptr<StreamConnector> connector_;
  LoopObjectPark object_park_;
};

}}

