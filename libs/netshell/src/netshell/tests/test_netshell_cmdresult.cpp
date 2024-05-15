#include "netshell/textualizer.h"
#include "netshell/untextualizer.h"
#include "netshell/serializer.h"
#include "netshell/unserializer.h"

#include "co/base/strings.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"
#include "co/base/bin_reader.h"
#include "co/base/bin_writer.h"

using namespace std;
using namespace co;
//using namespace co::async;
using namespace netshell;

namespace {

enum class WhatToExpect {
  kExpectMatch = 0,
  kExpectErrorInHeader,
  kExpectMismatchInHeader,
  kExpectErrorInBody,
  kExpectErrorInBodyEnd,
  kExpectMismatch
};

// TODO: Tests for parallel netshell

// Now hardcoded NsCmdResultTextualizerText; when there will be more types of
// factories, add it here
static void TestTextualizeUntextualize(
  const NsStatusDescriptorTable& status_descriptors,
  const NsCmdResult& src,
  WhatToExpect expect = WhatToExpect::kExpectMatch)
{
  string line;
  StringVector lines;
  NetshellError err;
  NsCmdResult dst;
  NsCmdResultTextualizer texer(status_descriptors, src);
  NsCmdResultUntextualizer untexer(status_descriptors, dst);
  // Check serialize / parse all (with body)
  texer.TextualizeToVector(lines);
  DCHECK(lines.size() > 0);
  // input first line from |src| to |dst|
  untexer.UntextualizeFirstLine(lines[0], err);
  if (expect == WhatToExpect::kExpectErrorInHeader) {
    DCHECK(err);
    return; // meaningless to continue
  } else {
    DCHECK(!err);
  }
  if (src.CompareHeaderFields(dst)) {
    // |src| and |dst| are EQUAL
    DCHECK(expect != WhatToExpect::kExpectMismatchInHeader);
  }
  else {
    // not equal
    DCHECK(expect == WhatToExpect::kExpectMismatchInHeader);
    return; // meaningless to continue
  }
  // input lines from |src| to |dst|
  for (size_t i=1; i<lines.size(); i++) {
    untexer.InputBodyLine(lines[i], err);
    if (expect == WhatToExpect::kExpectErrorInBody) {
      DCHECK(err);
    }
    else {
      DCHECK(!err);
    }
  }
  untexer.InputBodyEnd(err);
  if (expect == WhatToExpect::kExpectErrorInBodyEnd) {
    DCHECK(err);
    return; // meaningless to continue
  } else {
    DCHECK(!err);
  }

  // Smart compare
  bool equal = dst == src;

  if (expect == WhatToExpect::kExpectMatch) {
    DCHECK(equal);
  } else {
    DCHECK(expect == WhatToExpect::kExpectMismatch); // what else left?
    DCHECK(!equal);
  }
}

static void TestSerializeUnserialize(const NsStatusDescriptorTable& status_descriptors,
  const NsCmdResult& src)
{
  string buf;
  co::BinWriter writer(buf);
  NsCmdResultSerializer serer(status_descriptors, src);

  serer.Serialize(writer);
  co::BinReader reader(buf);

  NsCmdResult dst;
  NsCmdResultUnserializer unserer(status_descriptors, dst);
  NetshellError err;
  unserer.Unserialize(reader, err);

  DCHECK(!err);

  DCHECK(dst == src);
}

static void TestTextualizeSerialize(const NsStatusDescriptorTable& status_descriptors,
  const NsCmdResult& src)
{
  TestTextualizeUntextualize(status_descriptors, src, WhatToExpect::kExpectMatch);
  TestSerializeUnserialize(status_descriptors, src);
}

} // namespace



void test_netshell_cmdresult(TestInfo& test_info) {
  static int kCodeGood = 5001, kCodeBad = 5002;

  // Status descriptors for testing
  static NsStatusDescriptorTable sd = {
    {kCodeGood, {"CODE_GOOD", fCanHaveBody}},
    {kCodeBad, {"CODE_BAD", fCanNotHaveBody}}, // TODO: test it
  };

  using NsRes = NsCmdResult;

  // Test default
  TestTextualizeSerialize(sd, NsRes(kCodeGood));
  TestTextualizeSerialize(sd, NsRes(kCodeBad));

  // Check |ret_code| positive/negative
  TestTextualizeSerialize(sd, NsRes(kCodeGood, -1, NsResultType::kNone, 0));
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 0, NsResultType::kNone, 0));
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 1, NsResultType::kNone, 0));

  syslog(_INFO) << "Testing message type\n";
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 0, NsResultType::kMessage).WithMessageBody(""));
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 0, NsResultType::kMessage).WithMessageBody("hello world"));
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 0, NsResultType::kMessage).WithMessageBody(
    "I want pussy, sex, party, I have C++\nPython\nLinux\n\n")); // expect escaped msg

  syslog(_INFO) << "Testing text type\n";
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 0, NsResultType::kText).WithTextBody(""));
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 0, NsResultType::kText).WithTextBody("1")); // no newline
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 0, NsResultType::kText).WithTextBody("abc\ndef"));
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 0, NsResultType::kText).WithTextBody("\n"));
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 0, NsResultType::kText).WithTextBody("\n\n"));

  syslog(_INFO) << "Testing csv type\n";
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 0, NsResultType::kCsv).WithCsvBody({}));
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 0, NsResultType::kCsv).WithCsvBody(
    {
      { { "h1" }, { "h2" }, { "h3" } },
      { { "a1" }, { "a2" }, { "a3" } },
      { { "b1" }, { "b2" }, { "b3" } },
    }
  ));
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 0, NsResultType::kCsv).WithCsvBody(
    {
      { { "h1" }, { "h2" }, { "h3" }           },
      { { "a1" }, { "a2" },                    },
      { { "b1" }, { "b2" }, { "b3" }, { "b4" } },
    }
    ));
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 0, NsResultType::kCsv).WithCsvBody({}));

  return;

  // This now fails::::::::::::::::::::::::::::::::::::::
  syslog(_INFO) << "Testing subresult array type\n";
  TestTextualizeSerialize(sd, NsRes(kCodeGood, 0, NsResultType::kSubresultArray).WithSubresultArray(
    { NsCmdResult(), NsCmdResult(), NsCmdResult() }));


  // Wrong way tests
  // Test expresses the logic behind separately controlled header and body.
  // What could a user do and what he couldn't?
}

static void TestParse(const NsStatusDescriptorTable& status_descriptors,
  const StringVector& lines, const NsCmdResult& expected_result)
{
  NetshellError err;
  NsCmdResult cmd_result;

  syslog(_INFO) << "Testing the following lines:\n";
  for (auto& line : lines) {
    syslog(_INFO) << line << "\n";
  }
  syslog(_INFO) << "\n";

  NsCmdResultUntextualizer untexer(status_descriptors, cmd_result);

  untexer.Untextualize(lines, err);
  syslog(_INFO) << "After unTextualizing: err = " << err.MakeErrorMessage() << "\n";
  DCHECK(!err);
  syslog(_INFO) << "Textualized again:\n";
  StringVector lines2;

  NsCmdResultTextualizer texer(status_descriptors, cmd_result);
  texer.TextualizeToVector(lines2);
  for (auto& line : lines2) {
    syslog(_INFO) << line << "\n";
  }
  syslog(_INFO) << "\n";

  DCHECK(!err);
  DCHECK(cmd_result == expected_result);
}

/*
void test_netshell_cmdresult_parse(TestInfo& test_info) {
  // For future
  //100,CMD_EXECUTED,-10,ERRCODE,0,asio.misc:2
  //TestParse({"100,CMD_EXECUTED,-10,ERRCODE,0,asio.misc:2"}, NsCmdResult(kNsCmdExecuted,
  //          -1500, NsResultType::kMessage));
}*/


void test_netshell_cmdresult_ignore_extra_parts(TestInfo&) {
  static NsStatusDescriptorTable sd = {
    {2000, {"TWO_THOUNSAND", fCanHaveBody}},
    {3000, {"THREE_THOUSAND", fCanNotHaveBody}},
  };
  TestParse(sd, { "2000,TWO_THOUNSAND,-10,NONE,0" }, NsCmdResult(2000, -10, NsResultType::kNone));
  TestParse(sd, { "3000,THREE_THOUSAND,-10,NONE,0,xxx,My,Brain" }, NsCmdResult(3000, -10, NsResultType::kNone));
}




