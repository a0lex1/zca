#include "co/base/tests.h"

using namespace co;

//void test_zca_fuzz_test_case_generator(TestInfo& ti); // I don't need it right now.
//void test_zca_fuzz_test_case_generator_shift(TestInfo& ti);
void stresstest_zca_fuzz_case(TestInfo& ti);
void stresstest_zca_fuzz(TestInfo& ti);
void test_zca_fuzz_cmdexec_botlist(TestInfo&);
void test_zca_fuzz_flood_opts(TestInfo&);
void test_co_async_loop_object_set(TestInfo& ti);
void test_zca_fuzz_somexxx0(TestInfo& ti);
void test_cas_admin_soldier_opts(TestInfo&);
void test_bots_simp(TestInfo&); // RENAME!!!!!!!
void test_admins_simp(TestInfo&); // RENAME!!!!!!!
void test_zca_fuzz_case_u(TestInfo&);

co::TestTable zca_fuzz_test_table = {
  //ADD_TEST(test_zca_fuzz_test_case_generator),
  //ADD_TEST(test_zca_fuzz_test_case_generator_shift),
  ADD_TEST(stresstest_zca_fuzz_case),
  ADD_TEST(stresstest_zca_fuzz),
  ADD_TEST(test_zca_fuzz_cmdexec_botlist),
  ADD_TEST(test_zca_fuzz_flood_opts),
  ADD_TEST(test_co_async_loop_object_set),
  ADD_TEST(test_zca_fuzz_somexxx0),
  ADD_TEST(test_cas_admin_soldier_opts),
  ADD_TEST(test_bots_simp),
  ADD_TEST(test_admins_simp),
  ADD_TEST(test_zca_fuzz_case_u)
};





