#pragma once

// from github + modified

#include "co/common.h"

#include <string>
#include <limits>
#include <cstdint>

namespace co {

class BinWriter {
public:
  BinWriter(std::string& buf);
  BinWriter(std::string& buf, size_t max_to_write);
  BinWriter(void* buf_ptr, size_t max_to_write);

  bool WriteByteArray(const void* bytes_ptr, uint32_t bytes_len);
  bool WriteByteArray16(const void* bytes_ptr, uint16_t bytes_len);

  bool WriteByteArrayCastSize(const void* bytes_ptr, size_t sz);

  template <typename I>
  bool WriteInt(I i);

  bool WriteInt64(int64_t v) { return WriteInt(v); }
  bool WriteUint64(uint64_t v) { return WriteInt(v); }
  bool WriteInt32(int32_t v) { return WriteInt(v); }
  bool WriteUint32(uint32_t v) { return WriteInt(v); }
  bool WriteInt16(int16_t v) { return WriteInt(v); }
  bool WriteUint16(uint16_t v) { return WriteInt(v); }
  bool WriteInt8(int8_t v) { return WriteInt(v); }
  bool WriteUint8(uint8_t v) { return WriteInt(v); }

  bool WriteString(const wchar_t* str);
  bool WriteString(const char* str);
  bool WriteString(const std::wstring& str);
  bool WriteString(const std::string& str);
  bool WriteChar(char c) { return WriteInt(static_cast<uint8_t>(c)); }
  bool WriteChar(wchar_t c) { return WriteInt(static_cast<uint16_t>(c)); }

  size_t SpaceLeft() const;

  template <typename C>
  bool WriteBasicString(const std::basic_string<C>& str);

  bool WriteRaw(const void* raw_ptr, uint32_t raw_len);

  template <typename X>
  bool WriteWithSize(const X& x);

  template <typename X>
  bool WriteArray32bitSize(const X* arr, uint32_t num_entries);

private:
  template <typename X, typename SizeType>
  bool WriteArrayWithSize(const X* arr, SizeType num_entries);
  static uint32_t HashFromCategory(const std::string&);

private:
  size_t       cur_pos_;
  size_t       max_to_write_;
  std::string* pstr_buf_;
  union {
    void* as_ptr;
    char* as_char;
  } raw_buf_;
};

template <typename X>
bool co::BinWriter::WriteArray32bitSize(const X* arr, uint32_t num_entries)
{
  return WriteArrayWithSize<X, uint32_t>(arr, num_entries);
}

template <typename I>
bool co::BinWriter::WriteInt(I i)
{
  return WriteRaw(&i, sizeof(i));
}

template <typename C>
bool co::BinWriter::WriteBasicString(const std::basic_string<C>& str)
{
  return WriteArray32bitSize(str.c_str(),
                             static_cast<uint32_t>(str.length()));
}

template <typename X, typename SizeType>
bool co::BinWriter::WriteArrayWithSize(const X* arr, SizeType num_entries)
{
  SizeType sz = sizeof(X) * num_entries;
  if (SpaceLeft() < sz + sizeof(sz)) {
    return false;
  }
  bool written;
  written = WriteRaw(&sz, sizeof(sz));
  written = WriteRaw(arr, sz);
  return true;
}

template <typename X>
bool co::BinWriter::WriteWithSize(const X& x)
{
  uint32_t sz = sizeof(x);
  if (SpaceLeft() < sz + sizeof(sz)) {
    return false;
  }
  bool written;
  written = WriteRaw(&sz, sizeof(sz));
  written = WriteRaw(&x, sz);
  return true;
}

}
