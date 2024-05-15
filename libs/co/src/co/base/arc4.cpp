#include "co/base/arc4.h"

namespace co {

arc4::arc4(const void* key, size_t key_len) {
  rc4_init(&ctx_, key, key_len);
}

std::string arc4::crypt_all(const void* key,
                            size_t key_len,
                            const std::string& input_bin) {
  arc4 r(key, key_len);
  std::string ret;
  ret.resize(input_bin.length());
  r.crypt(input_bin.c_str(), input_bin.length(), (void*)ret.c_str());
  return ret;
}

void arc4::crypt_all(const void* key, size_t key_len, const void* input, size_t input_len, void* output)
{
  arc4 r(key, key_len);
  r.crypt(input, input_len, output);
}

void arc4::rc4_init(rc4_context* context, const void* key, size_t key_len) {
  unsigned int k;
  unsigned char *m;
  int i, j, a;
  context->X = 0;
  context->Y = 0;
  m = context->M;
  for (i = 0; i < 256; i++)
    m[i] = (unsigned char)i;
  j = k = 0;
  for (i = 0; i < 256; i++, k++) {
    if (k >= key_len) k = 0;
    a = m[i];
    j = (j + a + ((unsigned char*)key)[k]) & 0xFF;
    m[i] = m[j];
    m[j] = (unsigned char)a;
  }
}

void arc4::rc4_crypt_decrypt(
  rc4_context* context,
  size_t length,
  const void* input,
  void* output)
{
  int x, y, a, b;
  size_t i;
  unsigned char *m;
  x = context->X;
  y = context->Y;
  m = context->M;
  for (i = 0; i < length; i++) {
    x = (x + 1) & 0xFF; a = m[x];
    y = (y + a) & 0xFF; b = m[y];
    m[x] = (unsigned char)b;
    m[y] = (unsigned char)a;
    ((unsigned char*)output)[i] = (unsigned char)
      (((unsigned char*)input)[i] ^ m[(unsigned char)(a + b)]);
  }
  context->X = x;
  context->Y = y;
}

void arc4::crypt(const void* input_data, size_t input_len, void* output) {
  rc4_crypt_decrypt(&ctx_, input_len, input_data, output);
}

}

