#include "co/base/tests.h"
#include "co/base/bin_reader.h"
#include "co/base/bin_writer.h"

using namespace std;
using namespace co;

void test_bin_reader_error_handling(TestInfo& test_info) {
  string buf;

  BinWriter binw(buf);
  binw.WriteUint32(10);

  uint64_t v64;
  BinReader binr(buf);
  DCHECK(!binr.ReadUint64(v64));
  DCHECK(binr.GetParseError() == BinReader::eParseErrorIncomplete);
}
