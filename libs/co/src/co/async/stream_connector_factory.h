#pragma once

#include "co/async/stream_connector.h"

namespace co {
namespace async {

class StreamConnectorFactory {
public:
  virtual ~StreamConnectorFactory() = default;

  virtual Uptr<StreamConnector> CreateStreamConnector() = 0;
};

}}

