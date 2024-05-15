#pragma once

#include <stdint.h>
#include <stddef.h>

namespace co {
namespace detail {

size_t base64_get_decoded_alloc_size(const void* encoded, size_t encoded_size);

size_t base64_get_encoded_alloc_size(size_t decoded_size, bool newlines);

void base64_encode(
  const void *src, size_t len,
  void* out, size_t* out_len,
  bool newlines);

bool base64_decode(const void* src, size_t len,
  void* out, size_t* out_len);

}}
