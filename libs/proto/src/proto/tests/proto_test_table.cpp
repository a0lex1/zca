#include "co/base/tests.h"

using namespace co;

void test_proto(TestInfo& test_info);

co::TestTable proto_test_table = {
  ADD_TEST(test_proto)
};

