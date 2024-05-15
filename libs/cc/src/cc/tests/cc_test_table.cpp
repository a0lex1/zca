#include "co/base/tests.h"

using namespace co;

void test_cc_client_server_botlist_access(TestInfo&);
void test_cc_client_server_zombie_kill(TestInfo& test_info);
void test_cc_bot_id(TestInfo&);
void test_cc_proto_scenarios_srv_doublehshake(TestInfo&);
void test_cc_proto_scenarios_srv_badmsgs(TestInfo&);
void test_cc_proto_scenarios_srv_unknownmsgs(TestInfo&);
void test_cc_proto_scenarios_cli_writ_cmdres_err(TestInfo&);
void test_cc_proto_scenarios_cli_read_hshakeresult_err(TestInfo&);
void test_cc_proto_scenarios_cli_hshake_unexpect(TestInfo&);

co::TestTable cc_test_table = {
  ADD_TEST(test_cc_client_server_botlist_access),
  ADD_TEST(test_cc_client_server_zombie_kill),

  ADD_TEST(test_cc_bot_id),

  ADD_TEST(test_cc_proto_scenarios_srv_doublehshake),
  ADD_TEST(test_cc_proto_scenarios_srv_badmsgs),
  ADD_TEST(test_cc_proto_scenarios_srv_unknownmsgs),

  ADD_TEST(test_cc_proto_scenarios_cli_writ_cmdres_err),
  ADD_TEST(test_cc_proto_scenarios_cli_read_hshakeresult_err),
  ADD_TEST(test_cc_proto_scenarios_cli_hshake_unexpect),
};




