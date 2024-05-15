#include "netshell/untextualizer.h"
#include "netshell/ns_result_type_by_name.h"
#include "netshell/netshell_exception.h"

#include "co/base/csv/escape_unescape.h"
#include "co/base/strings.h"

using namespace std;

namespace netshell {

NsCmdResultUntextualizer::NsCmdResultUntextualizer(
  const NsStatusDescriptorTable& status_descriptors,
  NsCmdResult& ns_result)
  :
  status_descriptors_(status_descriptors), ns_result_(ns_result)
{
}

void NsCmdResultUntextualizer::UntextualizeFirstLine(const string& first_line, NetshellError& err) {
  ns_result_.Clear();
  StringVector line_parts;
  co::string_split(first_line, ",", line_parts);
  // code,status,ret_code,type,body_line_count
  if (line_parts.size() < 5) {
    // At least code,status,NAME must be present
    err = NetshellError(NetshellErrc::bad_part_count);
    return;
  }
  // code
  if (!co::string_to_uint(line_parts[0], ns_result_.status_code)) {
    err = NetshellError(NetshellErrc::bad_status_code);
    return;
  }
  // status (string)
  const string& status_name(line_parts[1]); // emphasize, field is skipped
  int found_status;
  if (!FindStatusByName(status_name.c_str(), found_status)) {
    err = NetshellError(NetshellErrc::status_name_not_found);
    return;
  }
  if (ns_result_.status_code != found_status) {
    err = NetshellError(NetshellErrc::status_name_mismatch);
    return;
  }
  // |ret_code|
  if (!co::string_to_int(line_parts[2], ns_result_.ret_code)) {
    err = NetshellError(NetshellErrc::bad_ret_code);
    return;
  }
  // |result_type|
  if (!NsResultTypeByName(line_parts[3], ns_result_.result_type)) {
    err = NetshellError(NetshellErrc::bad_result_type);
    return;
  }
  // |body_line_count|
  if (!co::string_to_uint(line_parts[4], ns_result_.body_line_count)) {
    err = NetshellError(NetshellErrc::bad_body_line_count);
    return;
  }
  // Handle extra row in first line
  auto found_it = status_descriptors_.find(ns_result_.status_code);
  if (found_it != status_descriptors_.end()) {
    //string& found_status_name(found_it->second.first);
    const NsFlag& found_flag = found_it->second.second;
    if (found_flag & fCanHaveBody) {
      switch (ns_result_.result_type) {
      case NsResultType::kMessage:
        // Message string goes 6 row (CSV rules) for MSG results
        if (line_parts.size() < 6) {
          err = NetshellError(NetshellErrc::bad_part_count);
          return;
        }
        ns_result_.message = co::csv::CsvUnescape(line_parts[5]);
        break;
      default:
        break;// compiller happy
        /*case NsResultType::kResultErrcode:
          if (line_parts.size() < 6) {
            err = NetshellError(NetshellErrc::bad_part_count);
            return;
          }
          errcode_field = ErrcodeFromString(CsvUnescape(line_parts[5]));
          break;*/
      }
    }
    else {
      DCHECK(found_flag & fCanNotHaveBody); // one of them should be set to detect errors
      err = NetshellError::NoError();
    }
  }
  else {
    err = NetshellError(NetshellErrc::unknown_status_code);
  }
}

void NsCmdResultUntextualizer::InputBodyLine(const string& line, NetshellError& err) {
  switch (ns_result_.result_type) {
  case NsResultType::kNone:
    err = NetshellErrc::unexpected_line;
    break;
  case NsResultType::kMessage:
    err = NetshellErrc::unexpected_line;
    break;
  case NsResultType::kText:
    if (ns_result_.text_lines.size() == ns_result_.body_line_count) {
      // All body lines already filled
      err = NetshellErrc::unexpected_line;
    }
    ns_result_.text_lines.push_back(line);
    break;
  case NsResultType::kCsv: {
    if (ns_result_.csv_rows.size() == ns_result_.body_line_count) {
      // All body rows already filled
      err = NetshellErrc::unexpected_line;
    }
    StringVector parts;
    co::string_split(line, ",", parts);
    ns_result_.csv_rows.push_back(parts);
    break;
    // Here can be something:
    //err = NetshellErrc::invalid_csv_char;
  }
  case NsResultType::kSubresultArray:
    if (ns_result_.subresult_array.size() == ns_result_.body_line_count) {
      // All body results already filled
    }
  default:
    NOTREACHED(); // > Except this rule, all other DCHECK()s are made as usual(arguments, etc.)
  }
}

void NsCmdResultUntextualizer::InputBodyEnd(NetshellError& err) {
  // This requires |result_type| to be set properly, otherwise NOTREACHED() will be made
  switch (ns_result_.result_type) {
  case NsResultType::kNone:
  case NsResultType::kMessage:
    err = NetshellError::NoError();
    break;
  case NsResultType::kText:
    if (ns_result_.text_lines.size() == ns_result_.body_line_count) {
      err = NetshellError::NoError();
    }
    else {
      err = NetshellErrc::unexpected_line;
    }
    break;
  case NsResultType::kCsv: {
    if (ns_result_.csv_rows.size() == ns_result_.body_line_count) {
      err = NetshellError::NoError();
    }
    else {
      err = NetshellErrc::unexpected_line;
    }
    break;
    // Here can be something:
    // err = NetshellErrc::invalid_csv_char;
  }
  case NsResultType::kSubresultArray:
    if (ns_result_.subresult_array.size() == ns_result_.body_line_count) {
      err = NetshellError::NoError();
    }
    else {
      err = NetshellErrc::unexpected_line;
    }
  default:
    NOTREACHED();
  }
}

void NsCmdResultUntextualizer::Untextualize(detail::StringReader& line_reader,
  NetshellError& err)
{
  string line;
  if (!line_reader.ReadString(line)) {
    err = NetshellError(NetshellErrc::unexpected_end);
    return;
  }
  UntextualizeFirstLine(line, err);
  if (err) {
    return;
  }
  while (line_reader.ReadString(line)) {
    InputBodyLine(line, err);
    if (err) {
      return;
    }
  }
  InputBodyEnd(err);
  if (err) {
    return;
  }
}

void NsCmdResultUntextualizer::Untextualize(const StringVector& lines, NetshellError& err) {
  netshell::detail::StringReaderFromContainer<StringVector> sr(lines);
  Untextualize(sr, err);
  // can't have trailing (unexpected) lines (Untextualize() would fail)
}

void NsCmdResultUntextualizer::Untextualize(const void* buf, size_t buf_len, NetshellError& err) {
  netshell::detail::StringReaderFromBuffer sr(buf, buf_len);
  Untextualize(sr, err);
}

NsCmdResult& NsCmdResultUntextualizer::Untextualize(const StringVector& lines) noexcept(false) {
  NetshellError err;
  Untextualize(lines, err);
  if (err) {
    throw NetshellException(err);
  }
  return ns_result_;
}

bool NsCmdResultUntextualizer::FindStatusByName(const char* status_name, int& found_status) {
  for (const auto& desc : status_descriptors_) {
    if (status_name == desc.second.first) {
      found_status = desc.first;
      return true;
    }
  }
  return false;
}

}

