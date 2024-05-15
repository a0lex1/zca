#pragma once

#include "co/net/endpoint.h"
#include "co/common_config.h"

#include <boost/date_time/time_duration.hpp>

struct BackendConfig {
  using time_duration = boost::posix_time::time_duration;
  using Endpoint = co::net::Endpoint;

  Endpoint admin_server_locaddr;
  Endpoint cc_server_locaddr;
  time_duration ping_interval; // 0 = disable
  uint32_t max_chunk_body_size{ 0 };

  BackendConfig(const Endpoint& _admin_server_locaddr = Endpoint(),
                const Endpoint& _cc_server_locaddr = Endpoint(),
                time_duration _ping_interval = boost::posix_time::seconds(120),
                uint32_t _max_chunk_body_size = co::common_config::kMaxChunkBodySize)
    :
    admin_server_locaddr(_admin_server_locaddr),
    cc_server_locaddr(_cc_server_locaddr),
    ping_interval(_ping_interval),
    max_chunk_body_size(_max_chunk_body_size)
  {
  }
};

struct BackendSeparationConfig {
  size_t admin_acceptor_threadgroup{ 0 };
  size_t admin_sessions_threadgroup{ 0 };
  size_t cc_acceptor_threadgroup{ 0 };
  size_t cc_sessions_threadgroup{ 0 };
  size_t modules_threadgroup{ 0 };
  size_t taskmgr_threadgroup{ 0 };
  size_t tss_threadgroup{ 0 };
};


