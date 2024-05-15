#include "co/test_kit/test_main.h"

extern co::TestTable co_listserver_stresstest_table;

int main(int argc, char* argv[]) {
  return test_main(argc, argv, {{"co_listserver_stresstest_table", co_listserver_stresstest_table}});
}
