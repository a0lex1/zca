#include "co/base/hex_dump.h"
#include "co/base/strings.h"
#include <limits>

//#undef max

//#pragma warning(disable:4996)

using namespace std;
using namespace co;

namespace co {

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long long uint64;

static const wstring kUnsafeCharReplacement = L".";

bool HexDump::address_section_enabled() const {
  return sec_set_ >= eAddressThanDigits && sec_set_ <= eAddressThanText;
}

bool HexDump::digits_section_enabled() const {
  return sec_set_ >= eDigits && sec_set_ <= eAddressThanDigitsThanText;
}

bool HexDump::text_section_enabled() const {
  return sec_set_ == eDigitsThanText ||
         sec_set_ == eAddressThanDigitsThanText ||
         sec_set_ == eAddressThanText ||
         sec_set_ == eTextOnly;
}

size_t HexDump::row_size() const {
  switch (row_type_) {
    case eByte: return sizeof(char);
    case eWord: return sizeof(short);
    case eDword: return sizeof(int);
    case eQword: return sizeof(uint64_t);
    default: return 0;
  }
}

size_t HexDump::row_max_len() const {
  switch (row_type_) {
    case eByte: return string_from_uint(numeric_limits<uchar>::max(), radix_).length();
    case eWord: return string_from_uint(numeric_limits<ushort>::max(), radix_).length();
    case eDword: return string_from_uint(numeric_limits<uint>::max(), radix_).length();
    case eQword: return string_from_uint64(numeric_limits<uint64>::max(), radix_).length();
    default: return 0;
  }
}

void HexDump::assert_params_valid() const {
  assert(rows_per_line_ > 0);
  assert(row_type_ == eByte ||
        row_type_ == eWord ||
        row_type_ == eDword ||
        row_type_ == eQword);
  assert(address_section_enabled() || text_section_enabled() || digits_section_enabled());
}

HexDump::HexDump(unsigned rows_per_line, section_set sec_set, bool widechar_text,
                           row_type _row_type, unsigned radix, bool leading_zeros)
           : rows_per_line_(rows_per_line),
             row_type_(_row_type),
             radix_(radix),
             sec_set_(sec_set),
             leading_zeros_(leading_zeros),
             widechar_text_(widechar_text)
{
  assert_params_valid();
}

co::HexDump& HexDump::RowsPerLine(unsigned rpl)
{
  rows_per_line_ = rpl;
  return *this;
}

//#define min(a,b) (a < b ? a : b)

#define ALIGN_DOWN(p, n) ((uintptr_t)(p) - ( (uintptr_t)(p) % (uintptr_t)(n) ))
#define ALIGN_UP(p, n)   ALIGN_DOWN((uintptr_t)(p) + (uintptr_t)(n) - 1, (n))

void HexDump::DoHexDumpW(wstring& append_to, const void* display_addr, const void* mem, size_t mem_len) const {
  assert_params_valid();
  //
  union {
    const void*    as_ptr;
    const uchar*   as_byte;
    const ushort*  as_word;
    const uint*    as_dword;
    const uint64*  as_qword;
    const char*    as_char;
    const wchar_t* as_wchar;
  } data;
  //
  size_t max_row_len = row_max_len();
  size_t bytes_in_line = row_size() * rows_per_line_;
  size_t aligned_len = ALIGN_DOWN(mem_len, row_size());
  size_t len_left = aligned_len;
  data.as_ptr = mem;
  //
  while (len_left) {
    bool got_print = false;
    if (address_section_enabled()) {
      got_print = true;
      const char* cur_display_addr =
        reinterpret_cast<const char*>(display_addr) +
        (reinterpret_cast<const char*>(data.as_ptr) -
         reinterpret_cast<const char*>(mem));

      append_to += co::string_printf(L"%p", cur_display_addr); //< good for x64 too
    }
    //
    size_t trailer_spaces = 0;
    size_t bytes_in_this_line = min(len_left, bytes_in_line);
    if (digits_section_enabled()) {
      if (got_print) {
        append_to += L" | ";
      } else {
        got_print = true;
      }
      size_t rows_in_this_line = bytes_in_this_line / row_size();
      size_t rows_in_full_line = bytes_in_line / row_size();
      size_t rows_lost = rows_in_full_line - rows_in_this_line;
      if (rows_lost) {
        trailer_spaces = rows_lost * max_row_len + (rows_lost);
      }
      for (size_t i=0; i<rows_in_this_line; i++) {
        wstring digit_str;
        switch (row_type_) {
          case eByte:
            digit_str = co::wstring_from_uint(data.as_byte[i], radix_);
            break;
          case eWord:
            digit_str = co::wstring_from_uint(data.as_word[i], radix_);
            break;
          case eDword:
            digit_str = co::wstring_from_uint(data.as_dword[i], radix_);
            break;
          case eQword:
            digit_str = co::wstring_from_uint64(data.as_qword[i], radix_);
            break;
          default: break;
        }
        //
        wstring row;
        for (size_t n=0; n<max_row_len-digit_str.length(); n++) {
          if (leading_zeros_) {
            row += L"0";
          } else {
            row += L" ";
          }
        }
        row += digit_str;
        append_to += row;
        if (i < rows_in_this_line-1) {
          append_to += L" ";
        }
      }
    }
    //
    if (text_section_enabled()) {
      if (got_print) {
        for (size_t n=0; n<trailer_spaces; n++) { append_to += L" "; }
        append_to += L" | ";
      } else {
        got_print = true;
      }
      size_t chars_in_this_line;
      if (widechar_text_) {
        chars_in_this_line = bytes_in_this_line / sizeof(wchar_t);
      } else {
        chars_in_this_line = bytes_in_this_line;
      }
      wstring text_buf;
      for (size_t i=0; i<chars_in_this_line; i++) {
        wchar_t char_to_append;
        if (widechar_text_) {
          char_to_append = data.as_wchar[i];
        } else {
          char_to_append = co::tchar_to_tchar<wchar_t, char>(data.as_char[i]);
        }
        if (co::is_ascii_graphic_char(char_to_append)) {
          text_buf.append(&char_to_append, 1);
        } else {
          text_buf += kUnsafeCharReplacement;
        }
      }
      append_to += text_buf;
      if (bytes_in_this_line % sizeof(wchar_t)) {
        append_to += L"-";
      }
    }
    //
    append_to += L"\n";
    len_left -= bytes_in_this_line;
    data.as_byte = &data.as_byte[bytes_in_this_line];
  }
}

wstring HexDump::DoHexDumpW(const void* display_addr, const void* mem, size_t mem_len) const {
  wstring wbuf;
  DoHexDumpW(wbuf, display_addr, mem, mem_len);
  return wbuf;
}

wstring HexDump::DoHexDumpW(const void* display_addr, const string& buf) const {
  return DoHexDumpW(display_addr, buf.c_str(), buf.length());
}

// ---

void HexDump::DoHexDump(string& append_to, const void* display_addr, const void* mem, size_t mem_len) const {
  wstring wresult;
  DoHexDumpW(wresult, display_addr, mem, mem_len);
  wstr_to_str_raw(wresult, append_to);
}

string HexDump::DoHexDump(const void* display_addr, const void* mem, size_t mem_len) const {
  string buf;
  DoHexDump(buf, display_addr, mem, mem_len);
  return buf;
}

std::string HexDump::DoHexDump(const void* display_addr, const std::string& buf) const {
  return DoHexDump(display_addr, buf.c_str(), buf.length());
}

co::HexDump& HexDump::SectionSet(section_set sset) {
  sec_set_ = sset;
  return *this;
}

}



