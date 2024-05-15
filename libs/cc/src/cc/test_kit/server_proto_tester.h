#pragma once

#include "proto/sync/proto_coro.h"

#include "cc/cc_server.h"

namespace cc {
namespace test_kit {

class ServerProtoTesterRunner;

// Server Proto Tester is a ProtoCoro that has an access to CcServerEvents
// and CcServer (for testing purposes)
class ServerProtoTester : public ProtoCoro, private CcServerEvents {
public:
  virtual ~ServerProtoTester() = default;

  using ProtoCoro::ProtoCoro;

protected:
  CcServer& GetCcServer() { return *cc_server_; }

  // Vanilla assert
  void MustSuccess(Errcode e);
  void MustSuccess(ProtoError pe);

private:
  void OnBotHandshakeComplete(Shptr<ICcBot> bot) override {}
  void OnBotRemovedFromList(Shptr<ICcBot> bot) override {}
  
  friend class ServerProtoTesterRunner;
  void SetCcServer(CcServer& cc_server) { cc_server_ = &cc_server; }
  CcServerEvents& GetEvents() { return *this; }

private:
  virtual void DoInteraction() override = 0;

private:
  CcServer* cc_server_{ nullptr };
};

}}

