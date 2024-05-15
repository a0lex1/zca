#pragma once

#include "zca/zca_common_config.h"

#include "co/net/endpoint.h"

struct AgentConfig {
  using time_duration = boost::posix_time::time_duration;

  cc::BotId bot_id;
  co::net::Endpoint back_remaddr;
  time_duration ping_interval;
  uint32_t max_chunk_body_size;

  bool hshake_postpone_enable;
  boost::posix_time::time_duration hshake_postpone_delay;

  std::string pki_factory_name;
  bool disable_cmd_sig_check;

  //AgentConfig() {}
  AgentConfig(const co::net::Endpoint& _backend_remote_addr = co::net::Endpoint(),
              time_duration _ping_interval = boost::posix_time::milliseconds(0),
              uint32_t _max_chunk_body_size = co::common_config::kMaxChunkBodySize,
              bool _hshake_postpone_enable = false,
              time_duration _hshake_postpone_delay = time_duration(),
              const std::string& _pki_factory_name="dummy",
              bool _disable_cmd_sig_check=false)
    :
    back_remaddr(_backend_remote_addr),
    ping_interval(_ping_interval),
    max_chunk_body_size(_max_chunk_body_size),
    hshake_postpone_enable(false),
    hshake_postpone_delay(_hshake_postpone_delay),
    pki_factory_name(_pki_factory_name),
    disable_cmd_sig_check(_disable_cmd_sig_check)
  {
  }
};

struct AgentSeparationConfig {
  size_t strand_threadgrp{ 0 };
};




