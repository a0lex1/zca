#pragma once

#include "netshell/ns_result_type.h"

#include "co/common.h"

#include <cstdint>
#include <string>
#include <vector>

namespace netshell {

struct NsCmdResult;
using NsCmdResultVector = std::vector<NsCmdResult>;

/* ******************************************************************************
* struct NsCmdResult
*
* Netshell result building/parsing/serializing swiss knife
*
*   You can input to it (by parsing), change header and body separately, serialize
* again and compare with another instance.
*
*   Instance of this class, when created with CTOR or Untextualize(), can't have a
* bad state.
* 
* CSV data special chars (, for example) is escaped automatically (if result_type == kCsv)
*
*   DCHECK()s work if <Header> fields are manually set incorrect or don't  match
* <Body> fields.
* *******************************************************************************
*/
struct NsCmdResult {
  using CsvRow = StringVector;

  // <Header>
  uint32_t                            status_code;
  int32_t                             ret_code;                // if >=kNone
  NsResultType                        result_type;             // if  |status| >= kCmdExecuted
  uint32_t                            body_line_count;

  // <Body>
  std::string                         message;
  StringVector                        text_lines;              // can be text_lines=std::move(lines_read_)
  std::vector<CsvRow>                 csv_rows;                // first can be header
  NsCmdResultVector                   subresult_array;
  Errcode                             errcode_field;

  // ---

  NsCmdResult(uint32_t _status_code = 0,
              int32_t _retcode = 0,
              NsResultType _result_type = NsResultType::kNone,
              uint32_t _body_line_count = 0);

  // Support syntax like NsCmdResult(kNsCmdExecuted, -1, kMessage).WithMessageBody("-1 returned")
  // With***Body() funcs fix |body_line_count|
  NsCmdResult& WithMessageBody(const std::string& _message, bool change_result_type_field = true);

  // IMPORTANT ! Don't complete your text with \n !
  // \n at the end is treated like an empty line !
  NsCmdResult& WithTextBody(const std::string& _full_text, bool change_result_type_field = true);
  // Alternatively, you can create text body with vector of lines
  NsCmdResult& WithTextBody(const StringVector& lines, bool change_result_type_field = true);
  // Or simpler
  NsCmdResult& WithTextBody(const char** lines, size_t line_count, bool change_result_type_field = true);
  NsCmdResult& WithCsvBody(const std::vector<CsvRow>& _csv_rows, bool change_result_type_field = true);
  NsCmdResult& WithSubresultArray(const NsCmdResultVector& _subresult_array, bool change_result_type_field = true);

  //static NsCmdResult StockFromErrcode(Errcode err);
  //static NsCmdResult StockFromNetshellError(const NetshellError&); // we don't need this

  void Clear() { *this = NsCmdResult(); }
  // Header and body are manipulated separately, sane check:
  //bool CheckHeaderMatchesBody() { //  // TODO *** //}

  // operator== is smart compare
  // This means it ignores body fields that are not for |result_type|
  // To compare including them, use CompareFull() (implement it)
  bool operator==(const NsCmdResult& r) const {
    return CompareHeaderFields(r) && CompareCorrespondingBodyFields(r);
  }
  bool operator!=(const NsCmdResult& r) const { return !operator==(r); }

  // Returns true if equal
  bool CompareHeaderFields(const NsCmdResult& r) const;

  // This doesn't check body fields not referenced by |result_type|
  bool CompareCorrespondingBodyFields(const NsCmdResult& r) const;
};

}
