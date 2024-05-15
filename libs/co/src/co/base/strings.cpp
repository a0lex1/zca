#include "co/base/strings.h"

#include <boost/algorithm/string.hpp>

#include <sstream>
#include <string>
#include <iostream>
#include <iomanip>

using namespace std;

namespace co {

void str_to_wstr_raw(const string& subject, wstring& wresult) {
  wresult = wstring(subject.begin(), subject.end());
}

wstring str_to_wstr_raw(const string& subject) {
  wstring wresult;
  str_to_wstr_raw(subject, wresult);
  return wresult;
}

void wstr_to_str_raw(const wstring& subject, string& result) {
  std::transform(subject.begin(), subject.end(), std::back_inserter(result), [](wchar_t c) {
    return (char)c;
                 });
}

string wstr_to_str_raw(const wstring& subject) {
  string str;
  wstr_to_str_raw(subject, str);
  return str;
}


string string_printf(const char* fmt, ...) {
  va_list vl;
  va_start(vl, fmt);
  return string_vprintf(fmt, vl);
}

wstring string_printf(const wchar_t* fmt, ...) {
  va_list vl;
  va_start(vl, fmt);
  return string_vprintf(fmt, vl);
}

string string_vprintf(const char* fmt, va_list vl) {
  char buf[co::kPrintBufSize];
  vsnprintf(buf, sizeof(buf), fmt, vl);
  return buf;
}

wstring string_vprintf(const wchar_t* fmt, va_list vl) {
  wchar_t buf[co::kPrintBufSize];
  C_vsnwprintf(buf, sizeof(buf)/sizeof(wchar_t), fmt, vl);
  return buf;
}

void string_split(const char* subject, const char* splitter_chars, vector<string>& parts) {
  boost::split(parts, subject, boost::is_any_of(splitter_chars));
}

void string_split(const string& subject, const string& splitter_chars, vector<string>& parts) {
  string_split(subject.c_str(), splitter_chars.c_str(), parts);
}

// -----------

bool string_to_uint(const char* subject, uint32_t& uval, int radix) {
  DCHECK(radix == 10 || radix == 16);
  char* endptr;
  uint32_t l = strtoul(subject, &endptr, radix);
  if (endptr != &subject[strlen(subject)]) {
    return false;
  }
  uval = l;
  return true;
}

bool string_to_uint(const string& subject, uint32_t& uval, int radix) {
  DCHECK(radix == 10 || radix == 16);
  return string_to_uint(subject.c_str(), uval, radix);
}

bool string_to_int(const char* subject, int32_t& uval, int radix) {
  DCHECK(radix == 10 || radix == 16);
  char* endptr;
  long l = strtol(subject, &endptr, radix);
  if (endptr != &subject[strlen(subject)]) {
    return false;
  }
  uval = l;
  return true;
}

bool string_to_int(const string& subject, int32_t& uval, int radix) {
  DCHECK(radix == 10 || radix == 16);
  return string_to_int(subject.c_str(), uval, radix);
}

bool string_to_uint64(const char* subject, uint64_t& uval, int radix) {
  DCHECK(radix == 10 || radix == 16);
  // Surprisingly,  strtoui64 believes that empty string is a DIGIT.
  // This is crazy. Obviously, it's not a DIGIT. Fix it.
  if (subject[0] == '\0') {
    return false;
  }
  char* endptr;
  uint64_t l = strtoull(subject, &endptr, radix);
  if (endptr != &subject[strlen(subject)]) {
    return false;
  }
  uval = l;
  return true;
}

bool string_to_size_t(const char* subject, size_t& uval, int radix) {
  DCHECK(radix == 10 || radix == 16);
  char* endptr;
  size_t l = strtoul(subject, &endptr, radix);
  if (endptr != &subject[strlen(subject)]) {
    return false;
  }
  uval = l;
  return true;
}

bool string_to_size_t(const string& subject, size_t& uval, int radix) {
  DCHECK(radix == 10 || radix == 16);
  return string_to_size_t(subject.c_str(), uval, radix);
}

bool string_to_uint64(const string& subject, uint64_t& uval, int radix) {
  DCHECK(radix == 10 || radix == 16);
  return string_to_uint64(subject.c_str(), uval, radix);
}

bool string_to_uint16(const char* subject, uint16_t& uval, int radix) {
  char* endptr;
  uint32_t l = strtoul(subject, &endptr, radix);
  if (endptr != &subject[strlen(subject)]) {
    return false;
  }
  if (l > numeric_limits<uint16_t>::max()) {
    // The value is too big
    return false;
  }
  uval = l;
  return true;
}

bool string_to_uint16(const std::string& subject, uint16_t& uval, int radix) {
  return string_to_uint16(subject.c_str(), uval, radix);
}

//

// string_from_xxx

string string_from_int(int32_t val, int radix) {
  switch (radix) {
  case 10:
    return string_printf("%i", val);
  case 16:
    return string_printf("%x", val);
  default:
    NOTREACHED();
  }
}

string string_from_uint(uint32_t val, int radix) {
  switch (radix) {
  case 10:
    return string_printf("%u", val);
  case 16:
    return string_printf("%x", val);
  default:
    NOTREACHED();
  }
}

string string_from_int64(int64_t val, int radix) {
  switch (radix) {
  case 10:
    return string_printf("%llu", val);
  case 16:
    return string_printf("%llx", val);
  default:
    NOTREACHED();
  }
}

string string_from_uint64(uint64_t val, int radix) {
  switch (radix) {
  case 10:
    return string_printf("%llu", val);
  case 16:
    return string_printf("%llx", val);
  default:
    NOTREACHED();
  }
}

// wstring_from_xxx

wstring wstring_from_int(int32_t val, int radix) {
  switch (radix) {
  case 10:
    return string_printf(L"%i", val);
  case 16:
    return string_printf(L"%x", val);
  default:
    NOTREACHED();
  }
}

wstring wstring_from_uint(uint32_t val, int radix) {
  switch (radix) {
  case 10:
    return string_printf(L"%u", val);
  case 16:
    return string_printf(L"%x", val);
  default:
    NOTREACHED();
  }
}

wstring wstring_from_int64(int64_t val, int radix) {
  switch (radix) {
  case 10:
    return string_printf(L"%llu", val);
  case 16:
    return string_printf(L"%llx", val);
  default:
    NOTREACHED();
  }
}

wstring wstring_from_uint64(uint64_t val, int radix) {
  switch (radix) {
  case 10:
    return string_printf(L"%llu", val);
  case 16:
    return string_printf(L"%llx", val);
  default:
    NOTREACHED();
  }
}

// ---

char* strnstr(const char* s, const char* find, size_t slen) {
  char c, sc;
  size_t len;

  if ((c = *find++) != '\0') {
    len = strlen(find);
    do {
      do {
        if (slen-- < 1 || (sc = *s++) == '\0')
          return (NULL);
      } while (sc != c);
      if (len > slen)
        return (NULL);
    } while (strncmp(s, find, len) != 0);
    s--;
  }
  return ((char*)s);
}

size_t string_replace_all_once(const string& subj, const string& what,
                          const string& replacement,
                          string& result_buf)
{
  size_t prev = 0, pos;
  if (subj.empty()) {
    return 0;
  }
  unsigned num_replaced = 0;
  for (;;) {
    pos = subj.find(what, prev);
    if (string::npos == pos) {
      result_buf.append(&subj[prev]);
      break;
    }
    result_buf.append(&subj[prev], pos - prev);
    result_buf.append(replacement);
    prev = pos + what.length();
    ++num_replaced;
  }
  return num_replaced;
}

string string_replace_all_once(const string& subj,
                          const string& what,
                          const string& replacement) {
  string result_buf;
  string_replace_all_once(subj, what, replacement, result_buf);
  return result_buf;
}


string string_to_hex(const string& binbuf) {

  string ret;

  static char const hex_chars[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

  for (auto& c : binbuf) {
    char const byte = c;
    ret += hex_chars[(byte & 0xF0) >> 4];
    ret += hex_chars[(byte & 0x0F) >> 0];
  }

  return ret;
}

/**
 * Return hexadecimal representation of the input binary sequence
 */
namespace {

// https://stackoverflow.com/questions/17261798/converting-a-hex-string-to-a-byte-array

using byte = unsigned char;

static int charToInt(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  return -1;
}

// Decodes specified HEX string to bytes array. Specified nBytes is length of bytes
// array. Returns -1 if fails to decode any of bytes. Returns number of bytes decoded
// on success. Maximum number of bytes decoded will be equal to nBytes. It is assumed
// that specified string is '\0' terminated.
bool hexStringToBytes(const char* str, byte* bytes, size_t nBytes, size_t& ret) {
  size_t nDecoded{ 0 };
  for (int i{ 0 }; str[i] != '\0' && nDecoded < nBytes; i += 2, nDecoded += 1) {
    if (str[i + 1] != '\0') {
      int m{ charToInt(str[i]) };
      int n{ charToInt(str[i + 1]) };
      if (m != -1 && n != -1) {
        bytes[nDecoded] = (m << 4) | n;
      }
      else {
        return false;
      }
    }
    else {
      return false;
    }
  }
  ret = nDecoded;
  return true;
}
}
bool string_from_hex(const std::string& hexstr,
                     void* binbuf,
                     size_t min_bytes,
                     size_t max_bytes,
                     size_t& ret) {
  DCHECK(min_bytes <= max_bytes);
  byte* bytes = new byte[max_bytes];
  if (!hexStringToBytes(hexstr.c_str(), bytes, max_bytes, ret)) {
    return false;
  }
  DCHECK(ret <= max_bytes);
  if (ret < min_bytes) {
    delete[] bytes;
    return false;
  }
  DCHECK(ret <= max_bytes);
  memcpy(binbuf, reinterpret_cast<char*>(bytes), ret);
  delete[] bytes;
  return true;
}

bool string_from_hex(const string& hexstr,
                     string& binstr,
                     size_t min_bytes,
                     size_t max_bytes) {
  binstr.resize(max_bytes);
  size_t ret;
  if (!string_from_hex(hexstr, const_cast<char*>(binstr.c_str()), min_bytes, max_bytes, ret)) {
    return false;
  }
  DCHECK(ret < binstr.length());
  binstr.resize(ret);
  return true;
}

void _string_to_upper(string& s) {
  // convert string to upper case
  for_each(s.begin(), s.end(), [](char& c) {
    c = toupper(c);
                });
}

string string_to_upper(const string& s) {
  string r(s);
  _string_to_upper(r);
  return r;
}


}


