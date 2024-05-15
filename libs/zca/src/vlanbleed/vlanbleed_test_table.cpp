#include "co/base/tests.h"

using namespace co;

void test_vlan_service(TestInfo&);
void test_vlan_native_ioc(TestInfo&);
void test_vlan_messages(TestInfo&);
void test_vlan_handle_table16(TestInfo&);
void test_vlan_handle_table64(TestInfo&);
void test_vlan_handle_table32_shptr(TestInfo&);
void test_vlan_handle_table32_shptr_rw(TestInfo&);

co::TestTable test_table = {
  ADD_TEST(test_vlan_service),
  ADD_TEST(test_vlan_native_ioc),
  ADD_TEST(test_vlan_messages),
  ADD_TEST(test_vlan_handle_table16),
  ADD_TEST(test_vlan_handle_table64),
  ADD_TEST(test_vlan_handle_table32_shptr),
  ADD_TEST(test_vlan_handle_table32_shptr_rw),
};

