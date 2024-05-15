#include "co/base/tests.h"

using namespace co;

void test_zca_echoargs(TestInfo&);
void test_zca_triplet(TestInfo&);
void test_zca_triplet_connerror(TestInfo&);
void test_zca_basecmd_cb_botlist(TestInfo&);
void test_zca_basecmd_cb_botlist_dummyfields(TestInfo&);
void test_zca_basecmd_cb_field_include(TestInfo& ti);
void test_zca_basecmd_cb_field_exclude(TestInfo& ti);

void test_zca_test_object(TestInfo& ti);
void test_zca_test_object_unrecov_exception(TestInfo& ti);
void test_zca_test_object_recov_exception(TestInfo& ti);

void test_zca_cb_cmd_needsig(TestInfo& ti);
void test_zca_cb_cmd_badsig(TestInfo& ti);
void test_zca_cb_cmd(TestInfo&);
void test_zca_cb_cmd_pprocess(TestInfo&);
void test_zca_cbf_cmd(TestInfo&);

void test_zca_cbf_cmd_floodbotlist(TestInfo&);

void test_zca_cmd_bkg(TestInfo&);

void test_zca_cb_instantstop(TestInfo&);
void test_zca_cb_instantstop_fakebackend(TestInfo&);
void test_zca_instantstop_simulation(TestInfo&);
void test_zca_cb_instantstop_serveronly(TestInfo&);
void test_zca_cb_instantstop_stopioc(TestInfo&);
void test_zca_cb_aborted_cmd(TestInfo&);

co::TestTable zca_test_table = {
  ADD_TEST(test_zca_echoargs),
  ADD_TEST(test_zca_triplet),
  ADD_TEST(test_zca_triplet_connerror),
  ADD_TEST(test_zca_basecmd_cb_botlist),
  ADD_TEST(test_zca_basecmd_cb_botlist_dummyfields),
  ADD_TEST(test_zca_basecmd_cb_field_include),
  ADD_TEST(test_zca_basecmd_cb_field_exclude),

  ADD_TEST(test_zca_cb_cmd_needsig),
  ADD_TEST(test_zca_cb_cmd_badsig),
  ADD_TEST(test_zca_cb_cmd),
  ADD_TEST(test_zca_cb_cmd_pprocess),
  ADD_TEST(test_zca_cbf_cmd),

  ADD_TEST(test_zca_cbf_cmd_floodbotlist),

  ADD_TEST(test_zca_cmd_bkg),

  ADD_TEST(test_zca_test_object),
  ADD_TEST(test_zca_test_object_unrecov_exception),
  ADD_TEST(test_zca_test_object_recov_exception),

  ADD_TEST(test_zca_cb_instantstop),
  ADD_TEST(test_zca_cb_instantstop_fakebackend),
  ADD_TEST(test_zca_instantstop_simulation),
  ADD_TEST(test_zca_cb_instantstop_serveronly),
  ADD_TEST(test_zca_cb_instantstop_stopioc),
  ADD_TEST(test_zca_cb_aborted_cmd),
};


