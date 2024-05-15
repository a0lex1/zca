#pragma once

#include "co/common.h"

namespace zca {
namespace test_kit {
namespace algo {

class ItemPicker {
public:
  virtual ~ItemPicker() = default;

  virtual void PickItems(size_t block_size, size_t seed,
                         std::vector<size_t>& picked_items) = 0;
};

// D = distance, N = num items
class ItemPickerSectorized : public ItemPicker {
public:
  virtual ~ItemPickerSectorized() = default;

  ItemPickerSectorized(size_t _D, size_t _N)
    :
    D_(_D), N_(_N)
  {
  }

  // Adds, not replaces
  void PickItems(size_t block_size, size_t slot,
                 std::vector<size_t>& picked_items) override;
private:
  size_t D_;
  size_t N_;
};


}}}

