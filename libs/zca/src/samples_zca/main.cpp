#include "co/test_kit/test_main.h"

extern co::TestTable zca_samples_table;

int main(int argc, char* argv[]) {
  return test_main(argc, argv, {{"zca_samples_table", zca_samples_table}}, true);
}
