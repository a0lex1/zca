#pragma once

#include "co/async/stream.h"
#include "co/base/ref_tracker.h"
#include "co/base/debug_tag_owner.h"
#include "co/xlog/define_logger_sink.h"

#include "co/net/endpoint.h"

#include <boost/asio/io_context.hpp>
#include <memory>
#include <iostream>

namespace co {
namespace async {

DECLARE_XLOGGER_SINK("acceptor", gCoAsyncStreamAcceptorLogSink);

class StreamAcceptorBase {
public:
  virtual ~StreamAcceptorBase() = default;

  using Endpoint = co::net::Endpoint;
  
  virtual bool IsOpen() = 0;
  virtual void Open(Errcode&) = 0;
  virtual void Bind(Endpoint, Errcode&) = 0;
  virtual void StartListening(Errcode&) = 0;
  virtual void GetLocalAddress(Endpoint&, Errcode&) = 0;

  // For example, TCP: if bound to 0.0.0.0, replaces it to 127.0.0.1
  virtual void GetLocalAddressToConnect(Endpoint&, Errcode&) = 0;

  virtual void Close() = 0;
  virtual void CancelAccept(Errcode&) = 0;

  // Throws if error
  void Open();
  void Bind(Endpoint);
  void StartListening();
  Endpoint GetLocalAddress();
  Endpoint GetLocalAddressToConnect();
  void CancelAccept();
};

// With error, encapsulates OS acceptor
class StreamAcceptor: public StreamAcceptorBase {
public:
  virtual ~StreamAcceptor() = default;

  StreamAcceptor(io_context& ioc) : ioc_(ioc) {}

  io_context& GetIoContext() { return ioc_; }

  // With error code
  virtual void AsyncAccept(
      Stream&,
      HandlerWithErrcode handler) = 0;

  DebugTagOwner& DebugTag() { return _dbg_tag_; }

private:
  io_context& ioc_;

  DebugTagOwner _dbg_tag_;
};

// Without error, wrapper
class StreamAcceptorNofail: public StreamAcceptorBase {
public:
  using AcceptHandler = Func<void()>;

  virtual ~StreamAcceptorNofail() = default;

  // Without error code
  virtual void AsyncAcceptNofail(
    Stream&,
    AcceptHandler handler) = 0;
};

class StreamAcceptorErrorLogic {
public:
  virtual ~StreamAcceptorErrorLogic() = default;

  static const bool kIssueAnotherAccept = true;

  // For example, OnError() can update counters and/or throw exception
  virtual bool OnError(Errcode err) = 0;
  virtual void OnSuccess() = 0;
};

class StreamAcceptorIgnoreLogic: public StreamAcceptorErrorLogic {
public:
  virtual ~StreamAcceptorIgnoreLogic() = default;

  bool OnError(Errcode err) override;
  void OnSuccess() override {
  }
};

/* // TODO?
class StreamAcceptorThrowRecoverableLogic : public StreamAcceptorErrorLogic {
public:
  virtual ~StreamAcceptorThrowRecoverableLogic() = default;

  bool OnError(Errcode err) override {
    throw co::RecoverableException();
  }
  void OnSuccess() override {
  }
};*/

class StreamAcceptorWithErrorLogic: public StreamAcceptorNofail {
public:
  virtual ~StreamAcceptorWithErrorLogic() = default;

  StreamAcceptorWithErrorLogic(
    StreamAcceptor& acpt_with_fail,
    StreamAcceptorErrorLogic& err_logic);

  bool IsOpen() override;
  void Open(Errcode&) override;
  void Bind(Endpoint, Errcode&) override;
  void StartListening(Errcode&) override;
  void GetLocalAddress(Endpoint&, Errcode&) override;
  void GetLocalAddressToConnect(Endpoint&, Errcode&) override;
  void Close() override;
  void CancelAccept(Errcode&) override;

  // If accept fails, |handler| may not get called
  void AsyncAcceptNofail(
    Stream& stm,
    AcceptHandler handler) override;

private:
  void HandleAcceptConsultErrorLogic(Errcode err, Stream& stm,
    AcceptHandler user_handler);

private:
  StreamAcceptor& acpt_with_fail_;
  StreamAcceptorErrorLogic& err_logic_;
};

}
}


