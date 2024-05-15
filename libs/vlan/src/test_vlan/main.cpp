#include "co/test_kit/test_main.h"

extern co::TestTable vlan_test_table;

int main(int argc, char* argv[]) {
  return test_main(argc, argv, {{"vlan_test_table", vlan_test_table}});
}
