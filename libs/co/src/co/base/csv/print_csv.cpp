#include "co/base/csv/print_csv.h"

#include <sstream>

using namespace std;

namespace co {
namespace csv {

void PrintCsvRows(const vector<StringVector>& csv_rows,
                  ostream& stm) {
  for (size_t irow = 0; irow < csv_rows.size(); irow++) {
    const StringVector& rows(csv_rows[irow]);
    for (size_t icol = 0; icol < rows.size(); icol++) {
      const string& col(rows[icol]);
      stm << col;
      if (icol + 1 != rows.size()) {
        // not last cycle
        stm << ",";
      }
    }
    stm << "\n";
  }
}

void PrintCsvRows(const vector<StringVector>& csv_rows,
             string& text) {
  stringstream ss;
  PrintCsvRows(csv_rows, ss);
  text = ss.str();
}


}}

