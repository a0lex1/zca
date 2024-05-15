
#pragma once

#include <string>

namespace co {

class arc4 {
public:
  arc4(const void* key_data, size_t key_len);

  void crypt(const void* input_data, size_t input_len, void* output);

  static void crypt_all(
    const void* key, size_t key_len,
    const void* input, size_t input_len, void* output);

  static std::string crypt_all(
    const void* key,
    size_t key_len,
    const std::string& input_bin);

private:
  struct rc4_context {
    int X;
    int Y;
    unsigned char M[256];
  };

  rc4_context ctx_;

private:
  static void rc4_init(rc4_context*, const void* key, size_t key_len);
  static void rc4_crypt_decrypt(
    rc4_context*, size_t length, const void* input,
    void* output);
};


}

