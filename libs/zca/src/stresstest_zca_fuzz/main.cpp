#include "co/test_kit/test_main.h"

extern co::TestTable zca_fuzz_test_table;

int main(int argc, char* argv[]) {
  return test_main(argc, argv, {{"zca_fuzz_test_table", zca_fuzz_test_table}});
}
