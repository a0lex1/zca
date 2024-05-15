#pragma once

#include "co/base/strings.h"

namespace co {
namespace csv {

  // This implementation DOESN'T CONSIDER "stri,ngs" in rows

static std::string CsvEscape(const std::string& src_msg) {
  // Escape , and \n because they can spoil text structure
  std::string dst_msg;
  dst_msg = co::string_replace_all_once(dst_msg, "&", "&amp;");
  dst_msg = co::string_replace_all_once(src_msg, ",", "&comma;");
  dst_msg = co::string_replace_all_once(dst_msg, "\n", "&lf;");
  return dst_msg;
}

static std::string CsvUnescape(const std::string& src_msg) {
  std::string dst_msg;
  dst_msg = co::string_replace_all_once(src_msg, "&comma;", ",");
  dst_msg = co::string_replace_all_once(dst_msg, "&lf;", "\n");
  dst_msg = co::string_replace_all_once(dst_msg, "&amp;", "&");
  return dst_msg;
}

}}

