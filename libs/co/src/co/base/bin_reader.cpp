#include "co/base/bin_reader.h"

using namespace std;

namespace co {

bool BinReader::ReadString(string& val, bool append /*= false*/)
{
  uint32_t len;
  if (!ReadUint32(len)) {
    // |parse_stat_| was set by read_uint32 -> read_fixed_number_of_pods
    return false;
  }
  if (BytesLeft() < len) {
    parse_err_ = eParseErrorIncomplete;
    Rollback(sizeof(uint32_t));
    return false;
  }
  char* sptr;
  if (append) {
    val.resize(val.length() + len);
    sptr = &val[val.length() - len];
  } else {
    if (val.length() > len) {
      val.clear();
    }
    val.resize(len);
    sptr = &val[0];
  }
  ReadFixedNumberOfPods(sptr, len); // Can't fail since we have already checked the size
  return true;
}

bool BinReader::ReadString(wstring& val, bool append /*= false*/)
{
  uint32_t len;
  if (!ReadUint32(len)) {
    // |parse_stat_| was set by read_uint32 -> read_fixed_number_of_pods
    return false;
  }
  if ((len % 2) != 0) {
    parse_err_ = eParseErrorMalformed;
    Rollback(sizeof(uint32_t));
    return false;
  }
  if (BytesLeft() < len) {
    parse_err_ = eParseErrorIncomplete;
    Rollback(sizeof(uint32_t));
    return false;
  }
  len /= sizeof(wchar_t);
  wchar_t* sptr;
  if (append) {
    val.resize(val.length() + len);
    sptr = &val[val.length() - len];
  } else {
    if (val.length() > len) {
      val.clear();
    }
    val.resize(len);
    sptr = &val[0];
  }
  ReadFixedNumberOfPods(sptr, len); // Can't fail since we have already checked the size
  return true;
}

}
