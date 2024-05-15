#include "co/base/detail/base64.h"

#include <string>

using namespace std;

namespace co {

  // High level base64 wrappers

static std::string decode64(
  const void* data, size_t data_len,
  const std::string& fallback_value = "")
{
  size_t alloc_size = co::detail::base64_get_decoded_alloc_size(data, data_len);
  string ret_string;
  ret_string.resize(alloc_size);
  size_t decoded_len;
  if (co::detail::base64_decode(data, data_len, (void*)ret_string.c_str(), &decoded_len)) {
    ret_string.resize(decoded_len);
  }
  else {
    ret_string = fallback_value;
  }
  return ret_string;
}

static std::string decode64(
  const std::string& val,
  const std::string& fallback_value = "")
{
  return decode64(val.c_str(), val.length(), fallback_value);
}

static std::string encode64(const void* data, size_t data_len, bool newlines) {
  size_t alloc_size = co::detail::base64_get_encoded_alloc_size(data_len, false);
  std::string ret_string;
  ret_string.resize(alloc_size);
  size_t encoded_size;
  co::detail::base64_encode(data,
    data_len, 
    (void*)ret_string.c_str(),
    &encoded_size,
    newlines
  );
  ret_string.resize(encoded_size);
  return ret_string;
}

static std::string encode64(const std::string& val, bool newlines) {
  return encode64(val.c_str(), val.length(), newlines);
}

}

