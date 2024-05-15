#include "co/base/cmdline/cmdline_section_split.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::cmdline;

static void TestSplit(const string& raw_cmdline,
                      const StringVector& expected_sections,
                      const string& splitter = " -- ") {
  StringVector actual_sections;
  CmdlineSectionSplit(raw_cmdline, actual_sections, splitter);

  syslog(_INFO) << "actual sections:\n";
  for (const auto& section : actual_sections) {
    cout << "\t" << section << "\n";
  }
  syslog(_INFO) << "expected sections:\n";
  for (const auto& section : expected_sections) {
    cout << "\t" << section << "\n";
  }

  DCHECK(actual_sections == expected_sections);

  syslog(_INFO) << "`" << raw_cmdline << "` test passed\n";
}

void test_co_cmdline_section_split(TestInfo& ti) {

  TestSplit("", { "" });

  TestSplit("--", { "--" });
  TestSplit(" -- ", { "", "" });
  TestSplit(" -- --", { "", "--" });
  TestSplit(" --  -- ", { "", "", "" });
  TestSplit(" --  -- a", { "", "", "a" });
  TestSplit(" -- -- --", { "", "-- --" });
  TestSplit(" --  --  -- ", { "", "", "", "" });

  TestSplit("abc", { "abc" });
  TestSplit("abc -- xey", { "abc", "xey" });
  TestSplit("abc -- xey -- ppp", { "abc","xey","ppp" });
  TestSplit("abc -- xey -- ", { "abc", "xey", "" });

  TestSplit("abc 'sex -- withpoor' hhh", {"abc 'sex -- withpoor' hhh"});
  TestSplit("abc \"hitler -- 222\" dog", {"abc \"hitler -- 222\" dog"});
  TestSplit("abc \"hitler -- '2'22\"", {"abc \"hitler -- '2'22\""});
  TestSplit("abc \"hitler\" -- \"333\"", { "abc \"hitler\"", "\"333\"" });
  TestSplit("abc 'hitler' -- \"333\"", { "abc 'hitler'", "\"333\"" });
  TestSplit("abc 'hitler' -- \"33'3'\"", { "abc 'hitler'", "\"33'3'\"" });
  TestSplit("'hitler' -- \"33'3'\"", { "'hitler'", "\"33'3'\"" });

  // ---

  //TestSplit("abc \"def ghj\" qwe 'r t y'", { "abc", "def ghj", "qwe", "r t y" }, " ");
}