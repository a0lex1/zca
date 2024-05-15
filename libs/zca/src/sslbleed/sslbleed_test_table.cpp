#include "co/base/tests.h"

using namespace co;

void test_ssl_simple(TestInfo&);

co::TestTable test_table = {
  ADD_TEST(test_ssl_simple),
};

