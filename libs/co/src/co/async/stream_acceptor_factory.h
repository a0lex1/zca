#pragma once

#include "co/async/stream_acceptor.h"

namespace co {
namespace async {

class StreamAcceptorFactory {
public:
  virtual ~StreamAcceptorFactory() = default;

  virtual Uptr<StreamAcceptor> CreateStreamAcceptor() = 0;
};

}}

