#include "zca/core/field_filter.h"

using namespace std;


namespace core {

void FieldFilter::CreateRemovalIndices(std::vector<size_t>& removal_indices)
{
  for (size_t nrow=0; nrow < header_rows_->size(); nrow++) {
    const string& row = header_rows_->at(nrow);
    if (include_columns_.size() != 0) {
      // If include_columns_ is non-empty, it ss assertic to us
      if (!(std::find(include_columns_.begin(), include_columns_.end(), row) != include_columns_.end())) {
        // Not found in Include Columns
        removal_indices.push_back(nrow);
      }
    }
    if ((std::find(exclude_columns_.begin(), exclude_columns_.end(), row) != exclude_columns_.end())) {
      // FOUND in exclude_columns_
      removal_indices.push_back(nrow);
    }    
  }
}

}
