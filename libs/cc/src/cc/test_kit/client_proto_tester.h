#pragma once

#include "proto/sync/proto_coro.h"

#include "cc/cc_client.h"

namespace cc {
namespace test_kit {

class ClientProtoTesterRunner;

class ClientProtoTester : public ProtoCoro, private CcClientEvents,
  public CcClientCommandDispatcher
{
public:
  virtual ~ClientProtoTester() = default;

  using ProtoCoro::ProtoCoro;

protected:
  CcClient& GetCcClient() { return *cc_client_; }

  // Vanilla assert
  void MustSuccess(Errcode e);
  void MustSuccess(ProtoError pe);

private:
  void OnClientHandshakeWritten() override {
  }
  void OnClientHandshakeReplyReceived() override {
  }

  void DispatchCommand(Uptr<std::string> cmd_opaque_data,
    std::string& cmd_result_opaque_data,
    EmptyHandler handler) override
  {
  }
  
  friend class ClientProtoTesterRunner;
  void SetCcClient(CcClient& cc_client) { cc_client_ = &cc_client; }
  CcClientEvents& GetEvents() { return *this; }
  CcClientCommandDispatcher& GetCommandDispatcher() { return *this; }

private:
  virtual void DoInteraction() override = 0;

private:
  CcClient* cc_client_{ nullptr };
};

}}

