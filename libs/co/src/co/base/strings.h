// Partially from third-party
#pragma once

#include "co/common.h"

#include <string>
#include <vector>
#include <codecvt>
#include <cstdarg>

namespace co {

static const size_t kPrintBufSize = 4096;

template <typename DestChar, typename SourceChar>
static inline DestChar tchar_to_tchar(SourceChar ansi_char) {
  return static_cast<DestChar>(ansi_char);
}
static inline char wchar_to_char(wchar_t c) {
  return tchar_to_tchar<char, wchar_t>(c);
}
static inline wchar_t char_to_wchar(char c) {
  return tchar_to_tchar<wchar_t, char>(c);
}

inline static bool is_ascii_graphic_char(wchar_t c) {
  return c >= 0x20 /* space */ && c <= 0x7e /* ~ */;
}
inline static bool is_ascii_graphic_char(char c) {
  return is_ascii_graphic_char(static_cast<wchar_t>(c));
}

void str_to_wstr_raw(const std::string& subject, std::wstring& wresult);
void wstr_to_str_raw(const std::wstring& subject, std::string& result);
static std::wstring str_to_wstr_raw(const std::string& subject);
static std::string wstr_to_str_raw(const std::wstring& subject); // No decoding, (char)wchr cast


std::string string_printf(const char* fmt, ...);
std::wstring string_printf(const wchar_t* fmt, ...);
std::string string_vprintf(const char* fmt, va_list vl);
std::wstring string_vprintf(const wchar_t* fmt, va_list vl);

void string_split(const char* subject, const char* splitter_chars, std::vector<std::string>& parts);
void string_split(const std::string& subject, const std::string& splitter_chars, std::vector<std::string>& parts);

// radix can be only 10 or 16 (hex)
bool string_to_uint(const char* subject, uint32_t& uval, int radix = 10);
bool string_to_uint(const std::string& subject, uint32_t& uval, int radix = 10);
bool string_to_int(const char* subject, int32_t& uval, int radix = 10);
bool string_to_int(const std::string& subject, int32_t& uval, int radix = 10);
bool string_to_size_t(const std::string& subject, size_t& uval, int radix = 10);
bool string_to_uint64(const char* subject, uint64_t& uval, int radix = 10);
bool string_to_uint64(const std::string& subject, uint64_t& uval, int radix = 10);
bool string_to_uint16(const char* subject, uint16_t& uval, int radix = 10);
bool string_to_uint16(const std::string& subject, uint16_t& uval, int radix = 10);

std::string string_from_int(int32_t val, int radix = 10);
std::string string_from_uint(uint32_t val, int radix = 10);
std::string string_from_int64(int64_t val, int radix = 10);
std::string string_from_uint64(uint64_t val, int radix = 10);

std::wstring wstring_from_int(int32_t val, int radix = 10);
std::wstring wstring_from_uint(uint32_t val, int radix = 10);
std::wstring wstring_from_int64(int64_t val, int radix = 10);
std::wstring wstring_from_uint64(uint64_t val, int radix = 10);

// stolen from Apple, lol
char* strnstr(const char* s, const char* find, size_t slen);

size_t string_replace_all_once(const std::string& subj, // returns number of replacements
                               const std::string& what,
                               const std::string& replacement,
                               std::string& result_buf);

std::string string_replace_all_once(const std::string& subj,
                                    const std::string& what,
                                    const std::string& replacement);

std::string string_to_hex(const std::string& binary_buffer);

bool string_from_hex(const std::string& hexstr,
                     void* binbuf,
                     size_t min_bytes,
                     size_t max_bytes,
                     size_t& ret);

bool string_from_hex(const std::string& hexstr,
                     std::string& binstr,
                     size_t min_bytes,
                     size_t max_bytes);

void _string_to_upper(std::string& s);

std::string string_to_upper(const std::string& s);

inline static std::string string_print_ptr(const void* ptr) {
  return string_printf("%p", ptr);
}

}

