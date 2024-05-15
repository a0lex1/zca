#include "co/base/file_get_contents.h"

using namespace std;

namespace co {

bool file_get_contents(const char* path, string& bin_data) {
  FILE* f = fopen(path, "rb");
  if (!f) {
    return false;
  }

  bool ret = false;

  fseek(f, 0, SEEK_END);
  size_t sz = ftell(f);

  fseek(f, 0L, SEEK_SET);

  bin_data.resize(sz);

  size_t nread = fread((void*)bin_data.c_str(), 1, sz, f);

  if (nread == sz) {
    ret = true;
  }

  fclose(f);
  return ret;
}

}
