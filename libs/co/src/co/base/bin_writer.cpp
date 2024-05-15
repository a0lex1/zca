#include "co/base/bin_writer.h"
#include "co/common.h"

using namespace std;

namespace co {

BinWriter::BinWriter(string& buf) {
  cur_pos_ = 0;
  pstr_buf_ = &buf;
  max_to_write_ = -1;
}

BinWriter::BinWriter(string& buf, size_t max_to_write) {
  cur_pos_ = 0;
  pstr_buf_ = &buf;
  max_to_write_ = max_to_write;
}

BinWriter::BinWriter(void* buf_ptr, size_t max_to_write) {
  cur_pos_ = 0;
  pstr_buf_ = 0;
  raw_buf_.as_ptr = buf_ptr;
  max_to_write_ = max_to_write;
}

size_t BinWriter::SpaceLeft() const {
  if (max_to_write_ == -1) {
    return -1;
  }
  else {
    return max_to_write_ - cur_pos_;
  }
}

bool BinWriter::WriteRaw(const void* raw_ptr, uint32_t raw_len) {
  if (SpaceLeft() < raw_len) {
    return false;
  }
  if (pstr_buf_ != 0) {
    pstr_buf_->append(static_cast<const char*>(raw_ptr), raw_len);
  } else {
    memcpy(&raw_buf_.as_char[cur_pos_], raw_ptr, raw_len);
  }
  cur_pos_ += raw_len;
  return true;
}

bool BinWriter::WriteByteArray(const void* bytes_ptr, uint32_t bytes_len) {
  const uint8_t* bytes_ptr_uchar = static_cast<const uint8_t*>(bytes_ptr);
  return WriteArray32bitSize(bytes_ptr_uchar, bytes_len);
}

bool BinWriter::WriteByteArray16(const void* bytes_ptr, uint16_t bytes_len) {
  const uint8_t* bytes_ptr_uchar = static_cast<const uint8_t*>(bytes_ptr);
  return WriteArrayWithSize<uint8_t, uint16_t>(bytes_ptr_uchar,
                                                          bytes_len);
}

bool BinWriter::WriteByteArrayCastSize(const void* bytes_ptr, size_t sz) {
  DCHECK(sz <= std::numeric_limits<uint32_t>::max());
  return WriteByteArray(bytes_ptr, static_cast<uint32_t>(sz));
}

bool BinWriter::WriteString(const wchar_t* str) {
  return WriteArray32bitSize(str, static_cast<uint32_t>(wcslen(str)));
}

bool BinWriter::WriteString(const char* str) {
  return WriteArray32bitSize(str, static_cast<uint32_t>(strlen(str)));
}

bool BinWriter::WriteString(const wstring& str) {
  return WriteBasicString(str);
}

bool BinWriter::WriteString(const string& str) {
  return WriteBasicString(str);
}

}
