#include "co/base/tests.h"

using namespace co;

void sample_zca_fuzz_item_picker(TestInfo& info);

co::TestTable zca_samples_table = {
  ADD_TEST(sample_zca_fuzz_item_picker)
};

