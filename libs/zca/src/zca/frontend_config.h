#pragma once

#include "co/net/endpoint.h"
#include "co/common_config.h"

struct FrontendConfig {
  using Endpoint = co::net::Endpoint;

  Endpoint frontend_server_locaddr;
  Endpoint bk_addr;

  std::string pki_factory_name;

  FrontendConfig(const Endpoint _frontend_server_locaddr = Endpoint(),
    const Endpoint& _bk_addr = Endpoint(),
    const std::string& _pki_fac_name="dummy")
    :
    frontend_server_locaddr(_frontend_server_locaddr),
    bk_addr(_bk_addr),
    pki_factory_name(_pki_fac_name)
  {
  }
};

struct FrontendSeparationConfig {
  size_t admin_acceptor_threadgroup{ 0 };
  size_t admin_sessions_threadgroup{ 0 };
  size_t modules_threadgroup{ 0 };
  size_t taskmgr_threadgroup{ 0 };
  size_t tss_threadgroup{ 0 };
};

