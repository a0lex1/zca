#include "zca/core/apply_removal_indices.h"

#include <algorithm>

namespace core {

void ApplyRemovalIndices(StringVector& input_strings,
  const std::vector<size_t>& indices)
{
  StringVector new_strings;
  for (size_t ninpstr=0; ninpstr<input_strings.size(); ninpstr++) {
    if (std::find(indices.begin(), indices.end(), ninpstr) != indices.end()) {
      // Found, skip this
      continue;
    }
    new_strings.push_back(input_strings[ninpstr]);
  }
  input_strings = std::move(new_strings);
}


}
