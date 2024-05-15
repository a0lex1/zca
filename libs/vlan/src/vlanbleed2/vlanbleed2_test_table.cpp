#include "co/base/tests.h"

using namespace co;

void test_co_async_services_shutdown(TestInfo&);
void test_co_async_services_simp(TestInfo&);
void test_vlan_service_shutdown(TestInfo&);

co::TestTable test_table = {
  ADD_TEST(test_co_async_services_shutdown),
  ADD_TEST(test_co_async_services_simp),
  ADD_TEST(test_vlan_service_shutdown),
};

