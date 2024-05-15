#pragma once

#include "co/net/parse_scheme_address.h"
#include "co/net/endpoint.h"

#include <map>

namespace co {
namespace net {

using EndpointFacFn = Func<Endpoint()>;

class EndpointSchemeRegistry {
 public:
  // Here could be functions to just create without initialize from string
  // But now to make it simpler there are only helpers which combines this 
  // It's not a design rule. Just helpers.
  void CreateEndpointForURI(const std::string& scheme_address,
                            Endpoint& ep,
                            Errcode& err,
                            std::string* scheme_save = nullptr) {
    err = NoError();
    std::string scheme, address;
    if (!co::net::parse_scheme_address(scheme_address, scheme, address)) {
      err = make_error_code(boost::system::errc::bad_address);
      return;
    }
    const auto& it = schemes_.find(scheme);
    if (it == schemes_.end()) {
      err = make_error_code(boost::system::errc::bad_address);
      return;
    }
    if (scheme_save) {
      *scheme_save = scheme;
    }
    // Call EndpointFacFn
    ep = it->second();
    // Automatically init if requested
    ep.FromString(address, err);
  }
  Endpoint CreateEndpointForURI(const std::string& scheme_address,
                                std::string* scheme_save = nullptr) {
    Endpoint ep;
    Errcode err;
    CreateEndpointForURI(scheme_address, ep, err, scheme_save);
    if (err) {
      BOOST_THROW_EXCEPTION(std::system_error(err));
    }
    return ep;
  }
  EndpointSchemeRegistry& RegisterScheme(const std::string& scheme,
                                         EndpointFacFn ep_fac_func)
  {
    DCHECK(schemes_.find(scheme) == schemes_.end());
    schemes_[scheme] = ep_fac_func;
    return *this;
  }

 private:
  std::map<std::string, EndpointFacFn> schemes_;
};

}}


