#pragma once

#include "co/async/thread_model.h"
#include "co/base/config.h"
#include <boost/date_time/posix_time/posix_time_duration.hpp>

namespace co {
namespace async {
namespace test_kit {

struct DbserverTestParams {
  size_t threadgrp_srv_db_strand; // todo
  size_t threadgrp_srv_acceptor; // ...
  size_t threadgrp_srv_sessions;
  size_t threadgrp_cli_conflood_sessions;
  size_t threadgrp_cli_writeflood_sessions;

  size_t num_conflood_sessions;
  size_t num_writer_sessions;
  size_t write_count_multipl;
  size_t write_size_multipl;
  size_t write_delay_multipl;

  bool restart_writer_sessions;
  bool restart_conflood_sessions;
  size_t server_stop_delay_ms;

  //boost::posix_time::time_duration checker_interval;

  size_t repeat_count; // default: 1, repeat single iter or all iters (if |single_iter|==0)

  bool stop_ioc_instead_of_server;

  struct {
    size_t iteration_number;
  } Hint;

public:
  void Print(std::ostream&, int num_spaces);
  std::string PrintOneLine() const;

  DbserverTestParams();
};

class DbserverTestParamsFromDictNoExcept : public co::ConfigFromDictNoexcept<DbserverTestParams, std::string, std::string> {
public:
  virtual ~DbserverTestParamsFromDictNoExcept() = default;

  using ConfigType = DbserverTestParams;
  using string = std::string;

  DbserverTestParamsFromDictNoExcept() {}
  DbserverTestParamsFromDictNoExcept(const DbserverTestParams& default_config,
    StringMap& dict, co::ConsumeAction consume_action,
    const std::vector<string>& required_fields = {});

private:
  void Parse();

private:
};

// ------------- with exception

class DbserverTestParamsFromDict : public co::ConfigFromDict<DbserverTestParamsFromDictNoExcept, std::string, std::string> {
public:
  using ConfigFromDict::ConfigFromDict;

  virtual ~DbserverTestParamsFromDict() = default;
};

}}}





