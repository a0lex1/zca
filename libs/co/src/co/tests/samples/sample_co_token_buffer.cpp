#include "co/base/tests.h"
#include "co/base/dict.h"
#include "co/base/buffered_tokenizer.h"
#include "co/base/dict.h"
#include "co/xlog/xlog.h"
#include <fstream>

using namespace std;
using namespace co;

// Usage: samples_co sample_co_token_buffer --file=text.txt
// "--file=c:\Program Files\Git\usr\share\vim\vim82\doc\os_390.txt"

void sample_co_token_buffer(co::TestInfo& test_info) {
  string file_path;
  OverrideFromDict<string, string, string>(test_info.opts_dict, "file", file_path, ConsumeAction::kConsume);

  BufferedTokenizer buf(1024*1024, "\n");
  char* space;
  size_t space_len;
  space = buf.GetBufferSpace(space_len);

  FILE* f;
#if (defined(_WIN32))
  if (fopen_s(&f, file_path.c_str(), "rb")) {
    syslog(_ERR) << "Can't read from binary file " << file_path << " (fopen_s)\n";
    return;
  }
#elif (defined(__APPLE__))
  if (!(f = fopen(file_path.c_str(), "rb"))) {
    syslog(_ERR) << "Can't read from binary file " << file_path << " (fopen)\n";
    return;
  }
#else
  // linux
  if (!(f = fopen64(file_path.c_str(), "rb"))) {
    syslog(_ERR) << "Can't read from binary file " << file_path << " (fopen64)\n";
    return;
  }
#endif
  size_t nread = fread(space, 1, space_len, f);
  fclose(f);
  buf.OnBufferSpaceFilled(nread);

  string token;
  while (buf.GetNextToken(token)) {
    cout << token << "<EOL>\n";
    token.clear();
  }
}


