#include "co/test_kit/test_main.h"

extern co::TestTable proto_test_table;

int main(int argc, char* argv[]) {
  return test_main(argc, argv, {{"proto_test_table", proto_test_table}});
}
