#include "co/test_kit/test_main.h"

extern co::TestTable co_test_table;
extern co::TestTable proto_test_table;
extern co::TestTable cc_test_table;
extern co::TestTable zca_test_table;
extern co::TestTable co_listserver_stresstest_table;
extern co::TestTable co_taskmgr_stresstest_table;
extern co::TestTable stresstest_zca_cmdexec_test_table;

int main(int argc, char* argv[]) {
  return test_main(argc, argv, {
    {"co_test_table", co_test_table},
    {"proto_test_table", proto_test_table},
    {"cc_test_table", cc_test_table},
    {"zca_test_table", zca_test_table},
    {"co_listserver_stresstest_table", co_listserver_stresstest_table},
    {"co_taskmgr_stresstest_table", co_taskmgr_stresstest_table},
    {"stresstest_zca_cmdexec_test_table", stresstest_zca_cmdexec_test_table},
    });
}
