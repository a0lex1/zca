#include "co/test_kit/test_main.h"

extern co::TestTable cc_test_table;

int main(int argc, char* argv[]) {
  return test_main(argc, argv, {{"cc_test_table", cc_test_table}});
}
