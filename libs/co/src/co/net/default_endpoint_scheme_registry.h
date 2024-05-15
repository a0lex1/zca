#pragma once

#include "co/net/endpoint_scheme_registry.h"

namespace co {
namespace net {

// Access to the global registry
EndpointSchemeRegistry& GetDefaultEpSchemeRegistry();

}}

