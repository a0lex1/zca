#include "zca/test_kit/algo/item_picker.h"

using namespace std;

namespace zca {
namespace test_kit {
namespace algo {

  void ItemPickerSectorized::PickItems(size_t block_size,
                                       size_t slot,
                                       std::vector<size_t>& picked_items) {
  picked_items.clear();

  size_t ofs = D_ * slot;
  for (size_t nstep=0; nstep < N_; nstep++) {
    if (ofs >= block_size) { // int overflow we don't care
      break;
    }
    picked_items.push_back(ofs);
    ofs += 1;
  }
}


}}}

