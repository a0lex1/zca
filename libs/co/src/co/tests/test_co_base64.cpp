#include "co/base/detail/base64.h"
#include "co/base/base64.h"
#include "co/base/tests.h"

using namespace std;
using namespace co;
using namespace co::detail;

namespace {

void detail_test_on(const void* data, size_t data_len, bool newlines, bool expect) {
  size_t ems = base64_get_encoded_alloc_size(data_len, newlines);
  char* p = (char*)malloc(ems);
  size_t out_len;
  base64_encode(data, data_len, p, &out_len, newlines);

  size_t decode_allocsize = base64_get_decoded_alloc_size(p, out_len);
  char* decoded = (char*)malloc(decode_allocsize);

  size_t decoded_len;
  bool actual = base64_decode(p, out_len, decoded, &decoded_len);
  if (!actual && expect) {
    NOTREACHED();
  }
  if (actual && !expect) {
    NOTREACHED();
  }

  if (actual) {
    DCHECK(decoded_len == data_len);
    DCHECK(!memcmp(data, decoded, data_len));
  }

  free(decoded);

  free(p);

}


void highlevel_test_on(const void* data, size_t data_len) {
  NOTREACHED();
}


void test_on(const void* data, size_t data_len, bool expect) {
  detail_test_on(data, data_len, true, expect);
  detail_test_on(data, data_len, false, expect);
  //highlevel_test_on(data, data_len);
}

}


void test_co_base64(TestInfo&) {
  test_on("", 0, false);
  test_on(" ", 1, true);
  test_on("abc", 3, true);
  test_on("\0", 1, true);
  test_on("a\0", 2, true);
  test_on("\xff\xee\xbb\xcc\xdd\xaa\x96", 7, true);
  test_on("\xff\xee\xbb aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbcccccccccccccccccccccccccccccccccccccccccccccccccccc777\xdd\xaa\x96", 7, true);
}
