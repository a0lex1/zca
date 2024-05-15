#include "co/base/cmdline/cmdline_section_split.h"

using namespace std;

namespace co {
namespace cmdline {

// returns false if unclosed quote/dquote
bool CmdlineSectionSplit(const string& raw_cmdline, StringVector& sections,
  const string& splitter)
{
/*
  if (raw_cmdline.length() == 0) {
    sections.push_back("");
    return true;
  }*/
  enum class InsideWhat { kNothing, kQuote, kDquote };
  InsideWhat inside_what = InsideWhat::kNothing;

  sections.clear();

  size_t part_start = 0;
  size_t len(raw_cmdline.length());

  sections.emplace_back("");
  for (size_t i = 0; i < len; i++) {
    const char& c = raw_cmdline[i];

    if (c == '\'') {
      if (inside_what == InsideWhat::kNothing) {
        // 'begin
        inside_what = InsideWhat::kQuote;
      }
      else {
        if (inside_what == InsideWhat::kQuote) {
          // end'
          inside_what = InsideWhat::kNothing;
        }
        else {
          DCHECK(inside_what == InsideWhat::kDquote);
        }
      }
      sections.back().append(&c, 1);
      continue;
    }
    if (c == '"') {
      if (inside_what == InsideWhat::kNothing) {
        // 'begin
        inside_what = InsideWhat::kDquote;
      }
      else {
        if (inside_what == InsideWhat::kDquote) {
          // end'
          inside_what = InsideWhat::kNothing;
        }
        else {
          DCHECK(inside_what == InsideWhat::kQuote);
        }
      }
      sections.back().append(&c, 1);
      continue;
    }
    if (inside_what == InsideWhat::kNothing) {
      // bla.exe 'aaa -- xxx' test -- lol "fufufu -- youyyy"
      // aaa -- 
      // -- 
      if (!strncmp(&c, splitter.c_str(), splitter.length())) {
        //sections.push_back(raw_cmdline.substr(part_start, i - part_start));
        sections.emplace_back("");
        // -1 because for() will increment |i| right after continue
        i += splitter.length()-1;
        part_start = i;
        continue;
      }
    }
    // basic char, add to current sections
    sections.back().append(&c, 1);
  }
/*
  if (part_start+1 < len) {
    //add the rest
    sections.push_back(raw_cmdline.substr(part_start));
  }
*/
  return inside_what == InsideWhat::kNothing; // unclosed quote?
}

}}

