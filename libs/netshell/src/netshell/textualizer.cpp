#include "netshell/textualizer.h"

#include "co/base/csv/escape_unescape.h"

using namespace std;

namespace netshell {

NsCmdResultTextualizer::NsCmdResultTextualizer(
  const NsStatusDescriptorTable& status_descriptors,
  const NsCmdResult& ns_result)
  :
  status_descriptors_(status_descriptors), ns_result_(ns_result)
{

}

void NsCmdResultTextualizer::TextualizeToVector(
  StringVector& lines,
  size_t max_line_length /* = -1 */,
  size_t max_line_count /* = -1 */)
{
  DCHECK(status_descriptors_.find(ns_result_.status_code) != status_descriptors_.end());

  // Full serialize.
  lines.clear();

  if (max_line_count == 0) {
    return;
  }

  // 1) Serialize header
  string first_line;
  TextualizeFirstLine(first_line);
  if (first_line.length() > max_line_length) {
    first_line.resize(max_line_length);
  }
  lines.push_back(first_line);

  // 2) Serialize body
  switch (ns_result_.result_type) {
  case NsResultType::kNone:
    break; // 0 lines
  case NsResultType::kMessage:
    break; // 0 lines
  case NsResultType::kText:
    for (size_t i=0; i<max_line_count-1; i++) {
      if (i == ns_result_.text_lines.size()) {
        break;
      }
      lines.push_back(ns_result_.text_lines[i]);
      if (lines.back().length() > max_line_length) {
        lines.back().resize(max_line_length);
      }
    }
    break;
  case NsResultType::kCsv: {
    // Print rows
    //
    // CSV automatically escaped!
    //
    for (size_t i=0; i<max_line_count-1; i++) {
      if (i == ns_result_.csv_rows.size()) {
        break;
      }
      const auto& csv_row(ns_result_.csv_rows[i]);
      string printed_row;
      bool first = true;
      for (const auto& col : csv_row) {
        if (first) {
          printed_row += co::csv::CsvEscape(col);
          first = false;
        }
        else {
          printed_row += "," + co::csv::CsvEscape(col);
        }
      }
      if (printed_row.length() > max_line_length) {
        printed_row.resize(max_line_length);
      }
      lines.push_back(printed_row);
    }
    break;
  }
  case NsResultType::kSubresultArray:
    break;
  default: NOTREACHED();
  }
}

void NsCmdResultTextualizer::TextualizeToStream(
  ostream& ostm,
  size_t max_line_length,
  size_t max_line_count)
{
  StringVector lines;
  TextualizeToVector(lines, max_line_length, max_line_count);
  for (auto& line : lines) {
    ostm << line << "\n";
  }
}

void NsCmdResultTextualizer::TextualizeToString(
  std::string& buf,
  size_t max_line_length,
  size_t max_line_count)
{
  stringstream ss;
  TextualizeToStream(ss, max_line_length, max_line_count);
  buf = ss.str();
}

void NsCmdResultTextualizer::TextualizeFirstLine(string& buf) {
  auto found_it = status_descriptors_.find(ns_result_.status_code);
  DCHECK(found_it != status_descriptors_.end());
  const string& status_name(found_it->second.first);
  const char* status_name_s(status_name.c_str());
  switch (ns_result_.result_type) {
  case NsResultType::kNone:
    buf = co::string_printf("%d,%s,%d,NONE,0", found_it->first, status_name_s, ns_result_.ret_code);
    break;
  case NsResultType::kMessage:
    buf = co::string_printf("%d,%s,%d,MSG,0,%s", found_it->first, status_name_s, ns_result_.ret_code, co::csv::CsvEscape(ns_result_.message).c_str());
    break;
  case NsResultType::kText:
    buf = co::string_printf("%d,%s,%d,TEXT,%d", found_it->first, status_name_s, ns_result_.ret_code, ns_result_.text_lines.size());
    break;
  case NsResultType::kCsv:
    buf = co::string_printf("%d,%s,%d,CSV,%d", found_it->first, status_name_s, ns_result_.ret_code, ns_result_.csv_rows.size());
    break;
  case NsResultType::kSubresultArray:
    buf = co::string_printf("%d,%s,%d,SUBRESULT_ARR,%d", found_it->first, status_name_s, ns_result_.ret_code, ns_result_.subresult_array.size());
    break;
  default: NOTREACHED();
  }
}

}

