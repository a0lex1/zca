#include "co/base/file_put_contents.h"

using namespace std;

namespace co {

bool file_put_contents(const char* path, const string& bin_data) {
  FILE* f = fopen(path, "wb");
  if (!f) {
    return false;
  }
  bool ret = false;
  size_t w = fwrite(bin_data.c_str(), 1, bin_data.length(), f);
  ret = (w == bin_data.length());
  fclose(f);
  return ret;
}

}
