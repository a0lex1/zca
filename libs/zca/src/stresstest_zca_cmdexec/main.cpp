#include "co/test_kit/test_main.h"

extern co::TestTable stresstest_zca_cmdexec_test_table;

int main(int argc, char* argv[]) {
  return test_main(argc, argv, {{"stresstest_zca_cmdexec_test_table", stresstest_zca_cmdexec_test_table}});
}
