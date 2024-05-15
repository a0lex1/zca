#include "co/test_kit/test_main.h"

extern co::TestTable co_samples_table;

int main(int argc, char* argv[]) {
  return test_main(argc, argv, {{"co_samples_table", co_samples_table}}, true);
}
