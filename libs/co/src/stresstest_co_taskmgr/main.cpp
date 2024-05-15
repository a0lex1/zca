#include "co/test_kit/test_main.h"

extern co::TestTable co_taskmgr_stresstest_table;

int main(int argc, char* argv[]) {
  return test_main(argc, argv, {{"co_taskmgr_stresstest_table", co_taskmgr_stresstest_table}});
}
