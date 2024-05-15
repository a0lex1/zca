#include "netshell/ns_cmd_result.h"
#include "co/base/strings.h"

using namespace std;

namespace netshell {

NsCmdResult::NsCmdResult(uint32_t _status_code, int _retcode, NsResultType _result_type, uint32_t _body_line_count)
  :status_code(_status_code), result_type(_result_type), ret_code(_retcode), body_line_count(_body_line_count)
{
}

NsCmdResult& NsCmdResult::WithMessageBody(const string& _message, bool change_result_type_field) {
  message = _message;
  body_line_count = 0;
  if (change_result_type_field) {
    result_type = NsResultType::kMessage;
  }
  return *this;
}

NsCmdResult& NsCmdResult::WithTextBody(const string& _full_text, bool change_result_type_field) {
  co::string_split(_full_text, "\n", text_lines);
  body_line_count = static_cast<uint32_t>(text_lines.size());
  if (change_result_type_field) {
    result_type = NsResultType::kText;
  }
  return *this;
}

NsCmdResult& NsCmdResult::WithTextBody(const StringVector& lines, bool change_result_type_field) {
  text_lines = lines;
  body_line_count = static_cast<uint32_t>(text_lines.size());
  if (change_result_type_field) {
    result_type = NsResultType::kText;
  }
  return *this;
}

NsCmdResult& NsCmdResult::WithTextBody(const char** lines, size_t line_count, bool change_result_type_field) {
  text_lines.clear();
  for (size_t i = 0; i < line_count; i++) {
    text_lines.emplace_back(lines[i]);
  }
  body_line_count = static_cast<uint32_t>(text_lines.size());
  if (change_result_type_field) {
    result_type = NsResultType::kText;
  }
  return *this;
}

NsCmdResult& NsCmdResult::WithCsvBody(const vector<CsvRow>& _csv_rows, bool change_result_type_field) {
  csv_rows = _csv_rows;
  body_line_count = static_cast<uint32_t>(csv_rows.size());
  if (change_result_type_field) {
    result_type = NsResultType::kCsv;
  }
  return *this;
}

NsCmdResult& NsCmdResult::WithSubresultArray(const NsCmdResultVector& _subresult_array, bool change_result_type_field) {
  subresult_array = _subresult_array;
  body_line_count = static_cast<uint32_t>(subresult_array.size());
  if (change_result_type_field) {
    result_type = NsResultType::kSubresultArray;
  }
  return *this;
}



bool NsCmdResult::CompareHeaderFields(const NsCmdResult& r) const {
  return
    this->status_code == r.status_code &&
    this->ret_code == r.ret_code &&
    this->result_type == r.result_type &&
    this->body_line_count == r.body_line_count;
}

bool NsCmdResult::CompareCorrespondingBodyFields(const NsCmdResult& r) const {
  if (this->result_type != r.result_type) {
    return false;
  }
  switch (result_type) {
  case NsResultType::kNone:
    return true; // enough
  case NsResultType::kMessage:
    return this->message == r.message;
  case NsResultType::kText:
    return this->text_lines == r.text_lines;
  case NsResultType::kCsv:
    return this->csv_rows == r.csv_rows;
  case NsResultType::kSubresultArray:
    if (subresult_array.size() != r.subresult_array.size()) {
      return false;
    }
    for (size_t i = 0; i < subresult_array.size(); i++) {
      if (subresult_array[i] != r.subresult_array[i]) {
        return false;
      }
    }
    return true; // arrays compared
  default:
    return false;
  }
}


}

