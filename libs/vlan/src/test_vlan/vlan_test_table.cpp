#include "co/base/tests.h"

using namespace co;

void test_vlan_todo(TestInfo&);
void test_vlan_service_shutdown(TestInfo&);

co::TestTable vlan_test_table = {
  ADD_TEST(test_vlan_todo),
  ADD_TEST(test_vlan_service_shutdown)
};


