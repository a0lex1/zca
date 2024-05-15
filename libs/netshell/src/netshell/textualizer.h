#pragma once

#include "netshell/status_descriptor_table.h"
#include "netshell/ns_cmd_result.h"

#include "co/base/textualizable.h"

#include <ostream>

namespace netshell {

class NsCmdResultTextualizer: public co::Textualizable {
public:
  NsCmdResultTextualizer(const NsStatusDescriptorTable& status_descriptors,
      const NsCmdResult& ns_result);
  
  // Body will be sexualized without taking |body_line_count_| header field in account.
  // |result_type| determines what body field will be sexualized (text_lines, csv_rows, etc.)

  using Textualizable::Textualize;

  void Textualize(std::ostream& append_to_stm) override {
    TextualizeToStream(append_to_stm, -1, -1);
  }

  void TextualizeFirstLine(std::string& buf);

  void TextualizeToString(
    std::string& buf,
    size_t max_line_length=-1,
    size_t max_line_count=-1);

  void TextualizeToVector(
    StringVector& lines,
    size_t max_line_length=-1,
    size_t max_line_count=-1);

  void TextualizeToStream(
    std::ostream& ostm,
    size_t max_line_length=-1,
    size_t max_line_count=-1);

private:
  const NsStatusDescriptorTable& status_descriptors_;
  const NsCmdResult& ns_result_;
};

}
