#include "./dbserver_test_params.h"
#include "co/async/configs/thread_model_config_from_dict.h"

using namespace std;
using namespace co;
using namespace co::async;
using namespace co::async::configs;
using boost::posix_time::time_duration;
using boost::posix_time::milliseconds;
using namespace boost::posix_time;

namespace co {
namespace async {
namespace test_kit {

DbserverTestParams::DbserverTestParams() {
  //tmodel_conf;
  threadgrp_srv_db_strand = 0;
  threadgrp_srv_acceptor = 0;
  threadgrp_srv_sessions = 0;
  threadgrp_cli_conflood_sessions = 0;
  threadgrp_cli_writeflood_sessions = 0;
  num_conflood_sessions = 1;
  num_writer_sessions = 1;
  write_count_multipl = 1;
  write_size_multipl = 1;
  write_delay_multipl = 1;
  restart_writer_sessions = false;
  restart_conflood_sessions = false;
  server_stop_delay_ms = 500;
  stop_ioc_instead_of_server = false;
}

//======================================

DbserverTestParamsFromDictNoExcept::DbserverTestParamsFromDictNoExcept(
  const DbserverTestParams& default_config,
  StringMap& dict,
  ConsumeAction consume_action,
  const std::vector<string>& required_fields /*= {}*/)
  :
  ConfigFromDictNoexcept(default_config,   dict, consume_action, required_fields)
{
  Parse();
}

void DbserverTestParamsFromDictNoExcept::Parse() {
  if (!OverrideFromConfigField<size_t>("conflooders", num_conflood_sessions)) {
    return;
  }
  if (!OverrideFromConfigField<size_t>("writers", num_writer_sessions)) {
    return;
  }
  if (!OverrideFromConfigField<size_t>("wcm", write_count_multipl)) {
    return;
  }
  if (!OverrideFromConfigField<size_t>("wsm", write_size_multipl)) {
    return;
  }
  if (!OverrideFromConfigField<size_t>("wdm", write_delay_multipl)) {
    return;
  }
  if (!OverrideFromConfigField<size_t>("stop-ms", server_stop_delay_ms)) {
    return;
  }
  if (!OverrideFromConfigField<bool>("stop-ioc", stop_ioc_instead_of_server)) {
    return;
  }
}

void DbserverTestParams::Print(ostream& stm, int num_spaces) {
  string tabs((size_t)num_spaces, ' ');

  stm << tabs << "threadgrp_srv_db_strand => " << threadgrp_srv_db_strand << "\n";
  stm << tabs << "threadgrp_srv_acceptor => " << threadgrp_srv_acceptor << "\n";
  stm << tabs << "threadgrp_srv_sessions => " << threadgrp_srv_sessions << "\n";
  stm << tabs << "threadgrp_cli_conflood_sessions => " << threadgrp_cli_conflood_sessions << "\n";
  stm << tabs << "threadgrp_cli_writeflood_sessions => " << threadgrp_cli_writeflood_sessions << "\n";
  stm << tabs << "threadgrp_srv_db_strand => " << threadgrp_srv_db_strand << "\n";

  stm << tabs << "num_conflood_sessions => " << num_conflood_sessions << "\n";
  stm << tabs << "num_writer_sessions => " << num_writer_sessions << "\n";
  stm << tabs << "write_count_multipl => " << write_count_multipl << "\n";
  stm << tabs << "write_size_multipl => " << write_size_multipl << "\n";
  stm << tabs << "write_delay_multipl => " << write_delay_multipl << "\n";

  stm << tabs << "restart_writer_sessions => " << std::boolalpha << restart_writer_sessions << "\n";
  stm << tabs << "restart_conflood_sessions => " << restart_conflood_sessions << "\n";
  stm << tabs << "server_stop_delay_ms => " << server_stop_delay_ms << " msec\n";

  stm << tabs << "stop_ioc_instead_of_server => " << std::boolalpha << stop_ioc_instead_of_server << "\n";
}

string DbserverTestParams::PrintOneLine() const
{
  stringstream ss;
  ss << "Iter# " << Hint.iteration_number
    << " " << (stop_ioc_instead_of_server ? "--stop-ioc" : "")
    << " --conflooders=" << num_conflood_sessions
    << " --writers=" << num_writer_sessions
    << " --wcm=" << write_count_multipl
    << " --wsm=" << write_size_multipl
    << " --wdm=" << write_delay_multipl
    << " --stop-ms=" << server_stop_delay_ms << "\n";
  return ss.str();
}

}}}
