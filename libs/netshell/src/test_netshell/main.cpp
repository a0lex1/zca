#include "co/test_kit/test_main.h"

extern co::TestTable netshell_test_table;

int main(int argc, char* argv[]) {
  return test_main(argc, argv, {{"netshell_test_table", netshell_test_table}});
}
