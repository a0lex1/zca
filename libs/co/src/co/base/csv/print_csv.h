#pragma once

#include "co/common.h"

#include <ostream>

namespace co {
namespace csv {

void PrintCsvRows(const std::vector<StringVector>& csv_rows,
                  std::ostream& stm);

void PrintCsvRows(const std::vector<StringVector>& csv_rows,
                  std::string& text);

}}