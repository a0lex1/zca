
#include "co/base/tests.h"
#include "co/base/bin_reader.h"
#include "co/base/bin_writer.h"

using namespace std;
using namespace co;

void test_co_bin_reader_writer_corrupt(TestInfo& test_info) {
  string buf;
  BinWriter writer(buf);
  DCHECK(writer.WriteUint32(0xfffee000));
  writer.WriteInt64(0xde4dbeafd3adbadd);

  BinReader reader(buf);
  const uint8_t* bytes;
  uint32_t bytes_len;
  DCHECK(!reader.ReadByteArray(bytes, &bytes_len));
}

void test_co_bin_reader_writer(TestInfo& test_info) {

  int64_t int64;
  uint64_t uint64;
  int32_t int32;
  uint32_t uint32;
  int16_t int16;
  uint16_t uint16;
  int8_t int8;
  uint8_t uint8;
  string str;
  wstring wstr;
  wchar_t wchr;

  string buf;

  BinWriter writer(buf);

  writer.WriteInt64(-0xF12345678901);
  DCHECK(BinReader(buf).ReadInt64(int64));
  DCHECK(BinReader(buf.c_str(), buf.length()).ReadInt64(int64)); // once
  DCHECK(int64 == -0xF12345678901);
  buf.clear();

  writer.WriteUint64(0x55345078901555);
  DCHECK(BinReader(buf).ReadUint64(uint64));
  DCHECK(uint64 == 0x55345078901555);
  buf.clear();

  writer.WriteInt32(-392105);
  DCHECK(BinReader(buf).ReadInt32(int32));
  DCHECK(int32 == -392105);
  buf.clear();

  writer.WriteUint32(0xabcdef);
  DCHECK(BinReader(buf).ReadUint32(uint32));
  DCHECK(uint32 == 0xabcdef);
  buf.clear();

  writer.WriteInt16(-5632);
  DCHECK(BinReader(buf).ReadInt16(int16));
  DCHECK(int16 == -5632);
  buf.clear();

  writer.WriteUint16(0xfdef);
  DCHECK(BinReader(buf).ReadUint16(uint16));
  DCHECK(uint16 == 0xfdef);
  buf.clear();

  writer.WriteInt8(-55);
  DCHECK(BinReader(buf).ReadInt8(int8));
  DCHECK(int8 == -55);
  buf.clear();

  writer.WriteUint8(199);
  DCHECK(BinReader(buf).ReadUint8(uint8));
  DCHECK(uint8 == 199);
  buf.clear();

  // ---------------------------------

  writer.WriteString(L"hello");
  DCHECK(BinReader(buf).ReadString(wstr));
  DCHECK(wstr == L"hello");
  buf.clear();

  writer.WriteString("yyy");
  DCHECK(BinReader(buf).ReadString(str));
  DCHECK(str == "yyy");
  buf.clear();

  writer.WriteChar('~');
  DCHECK(BinReader(buf).ReadUint8(uint8));
  DCHECK(uint8 == '~');
  buf.clear();

  writer.WriteChar(L'~');
  DCHECK(BinReader(buf).ReadUint16(*reinterpret_cast<uint16_t*>(&wchr)));
  DCHECK(wchr == L'~');
  buf.clear();
}
