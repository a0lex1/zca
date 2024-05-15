#include "co/test_kit/test_main.h"

extern co::TestTable co_test_table;

int main(int argc, char* argv[]) {
  return test_main(argc, argv, {{"co_test_table", co_test_table}});
}
