#include "co/net/endpoint_scheme_registry.h"

namespace co {
namespace net {

EndpointSchemeRegistry& GetDefaultEpSchemeRegistry() {
  static EndpointSchemeRegistry inst;
  return inst;
}

}}
