#pragma once

#include <tabulate/table.hpp>
#include <tabulate/markdown_exporter.hpp>

#include "co/common.h"

namespace co {
namespace csv {

static void CsvToTabulate(const std::vector<StringVector>& csv_table, tabulate::Table& table) {
  using Table = tabulate::Table;
  using Row_t = tabulate::Table::Row_t;
  for (const StringVector& csv_row : csv_table) {
    Row_t new_row;
    for (const std::string& cell : csv_row) {
      new_row.push_back(cell);
    }
    table.add_row(new_row);
  }
}

static void CsvToTabulateDump(const std::vector<StringVector>& csv_table, std::string& out_text) {
  using Table = tabulate::Table;
  tabulate::Table table;
  CsvToTabulate(csv_table, table);
  tabulate::MarkdownExporter exporter;
  out_text = exporter.dump(table);
}
 
}}
