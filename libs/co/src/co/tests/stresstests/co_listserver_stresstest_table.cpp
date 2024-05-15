#include "co/base/tests.h"

using namespace co;

void stresstest_co_async_dbserver_u(TestInfo& info); // for user params (cmdline)
void stresstest_co_async_dbserver_easy(TestInfo& info);
void stresstest_co_async_dbserver_hard(TestInfo& info);


co::TestTable co_listserver_stresstest_table = {
  ADD_TEST(stresstest_co_async_dbserver_u),
  ADD_TEST(stresstest_co_async_dbserver_easy),
  ADD_TEST(stresstest_co_async_dbserver_hard),
};

