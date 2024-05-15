#include "co/base/tests.h"

using namespace co;

void stresstest_co_async_task_manager_u(TestInfo& ti);
void stresstest_co_async_task_manager1(TestInfo&);
void stresstest_co_async_task_manager2(TestInfo&);
void stresstest_co_async_task_manager3(TestInfo&);
void stresstest_co_async_task_manager_cases(TestInfo&); 

co::TestTable co_taskmgr_stresstest_table = {
  ADD_TEST(stresstest_co_async_task_manager_u),
  ADD_TEST(stresstest_co_async_task_manager1),
  ADD_TEST(stresstest_co_async_task_manager2),
  ADD_TEST(stresstest_co_async_task_manager3),
  ADD_TEST(stresstest_co_async_task_manager_cases)
};



