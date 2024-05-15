#include "co/base/tests.h"

using namespace co;

void test_netshell(TestInfo&);
void test_netshell_cmdresult(TestInfo&);
void test_netshell_cmdresult_ignore_extra_parts(TestInfo&);

void test_netshell_para_executor(TestInfo&);

co::TestTable netshell_test_table = {
  ADD_TEST(test_netshell),
  ADD_TEST(test_netshell_cmdresult),
  ADD_TEST(test_netshell_cmdresult_ignore_extra_parts),

  ADD_TEST(test_netshell_para_executor)
};


