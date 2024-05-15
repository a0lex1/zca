#pragma once

#include "netshell/status_descriptor_table.h"
#include "netshell/netshell_error.h"
#include "netshell/ns_cmd_result.h"

#include "netshell/detail/string_reader.h"

#include "co/common.h"

namespace netshell {

class NsCmdResultUntextualizer {
public:
  NsCmdResultUntextualizer(const NsStatusDescriptorTable& status_descriptors,
    NsCmdResult& ns_result);
  
  void Untextualize(const StringVector& lines, NetshellError& err);
  void Untextualize(const void* buf, size_t buf_len, NetshellError& err);
  NsCmdResult& Untextualize(const StringVector& lines) noexcept(false);
  void Untextualize(detail::StringReader& line_reader, NetshellError& err);

  // Applies to <Header> fields (|status_code|, |result_type|, |ret_code|)
  void UntextualizeFirstLine(const std::string& first_line, NetshellError& err);

  // Applies to <Body> depending on |result_type|
  // Consumes |body_line_count_| lines into body field of corresponding type
  // Contents are meant to come from insecure peer (malformed lines are handled),
  //   but require |result_type| to be set
  void InputBodyLine(const std::string& line, NetshellError& err);
  void InputBodyEnd(NetshellError& err);

private:
  bool FindStatusByName(const char* status_name, int& found_status);

private:
  const NsStatusDescriptorTable& status_descriptors_;
  NsCmdResult& ns_result_;
};

}

