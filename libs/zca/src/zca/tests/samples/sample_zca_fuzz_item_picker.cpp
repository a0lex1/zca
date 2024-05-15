#include "zca/test_kit/algo/item_picker.h"

#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace zca::test_kit::algo;

void test_zca_fuzz_item_picker(TestInfo& ti) {

}

namespace {
  static void TestOn(size_t block_size, size_t num_slots,
                     size_t distance, size_t count,
                     size_t repeat_count,
                     bool interactive = false) {
    //
    vector<size_t> items;

    auto printitems = [&]() {
      stringstream ss;
      for (size_t i = 0; i < items.size(); i++) {
        ss << hex << items[i] << "\t";
      }
      if (interactive) {
        syslog(_INFO) << ss.str() << "\n";
      }
    };


    for (size_t i = 0; i < block_size; i++) {
      items.push_back(0);
    }

    Uptr<ItemPicker> picker = make_unique<ItemPickerSectorized>(distance, count);

    for (size_t nrep = 0; nrep< repeat_count; nrep++) {
      for (size_t slot = 0; slot < num_slots; slot++) {

        vector<size_t> picked_items;
        picker->PickItems(items.size(), slot, picked_items);

        for (size_t i = 0; i < picked_items.size(); i++) {
          size_t idx = picked_items[i];
          items[idx] = slot; // last access from |slot|
        }
        printitems();
      }
      if (interactive) {
        std::getchar();
      }
    }
  }
}

void sample_zca_fuzz_item_picker_all(TestInfo& ti) {
  for (size_t bs = 1; bs < 20; bs += 3) {
    for (size_t ns = 1; ns < 20; ns += 3) {
      for (size_t d = 1; d < 10; d++) {
        for (size_t n = 1; n < 10; n++) {
          syslog(_INFO) << "[ " << bs << "/" << ns << "/" << d << "/" << n << " ]\n";
          TestOn(bs, ns, d, n, 10);
        }
      }
    }
  }
}

void sample_zca_fuzz_item_picker(TestInfo& ti) {
  TestOn(20, 11, 3, 4, 15, true);
}
