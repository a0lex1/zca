#pragma once

#include "co/common.h"

namespace core {

class FieldFilter {
public:
  FieldFilter() {}

  void AddInclude(const std::string& str) {
    include_columns_.push_back(str);
  }
  void AddExclude(const std::string& str) {
    exclude_columns_.push_back(str);
  }

  void SetHeaderRow(const StringVector& header_rows) {
    header_rows_ = &header_rows;
  }

  void CreateRemovalIndices(std::vector<size_t>& removal_indices);

private:
  StringVector include_columns_;
  StringVector exclude_columns_;
  const StringVector* header_rows_{ nullptr };
};

}
