#include "./handle_table.h"

#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace co;
using namespace std;

void test_handle_table16(TestInfo&) {
  HandleTable<uint16_t, string> ht(8);
  uint16_t h1, h2, h3, _h;
  DCHECK(ht.CreateHandle(h1, "aaa") && (h1 == 0));
  DCHECK(ht.CreateHandle(h2, "bbb") && (h2 == 1));
  DCHECK(ht.CreateHandle(h3, "ccc") && (h3 == 2));
  DCHECK(ht.CloseHandle(h2));
  DCHECK(ht.CreateHandle(_h, "xxx") && (_h == 1)); // _h replaces h2
  DCHECK(ht.CreateHandle(_h, "ccc") && (_h == 3)); // 2 already used by "ccc"

  // now have 4 items, add more 4
  for (int i = 0; i < 4; i++) {
    DCHECK(ht.CreateHandle(_h, "NNN"));
    syslog(_INFO) << _h << "\n";
  }
  // now full
  DCHECK(!ht.CreateHandle(_h, "404"));

  // let relax, close h1
  ht.CloseHandle(h1);
  // now can insert
  DCHECK(ht.CreateHandle(_h, "jjj"));
}

void test_handle_table64(TestInfo&) {
  HandleTable<uint64_t, string> ht(8);
  uint64_t h1, h2, h3, _h;
  DCHECK(ht.CreateHandle(h1, "aaa") && (h1 == 0));
  DCHECK(ht.CreateHandle(h2, "bbb") && (h2 == 1));
  DCHECK(ht.CreateHandle(h3, "ccc") && (h3 == 2));
  DCHECK(ht.CloseHandle(h2));
  DCHECK(ht.CreateHandle(_h, "xxx") && (_h == 1)); // _h replaces h2
  DCHECK(ht.CreateHandle(_h, "ccc") && (_h == 3)); // 2 already used by "ccc"

  // now have 4 items, add more 4
  for (int i = 0; i < 4; i++) {
    DCHECK(ht.CreateHandle(_h, "NNN"));
    syslog(_INFO) << _h << "\n";
  }
  // now full
  DCHECK(!ht.CreateHandle(_h, "404"));

  // let relax, close h1
  ht.CloseHandle(h1);
  // now can insert
  DCHECK(ht.CreateHandle(_h, "jjj"));
}

void test_handle_table32_shptr(TestInfo&) {
  HandleTable<unsigned, Shptr<string>> ht(8);
  unsigned h1, h2, h3, _h;
  DCHECK(ht.CreateHandle(h1, make_shared<string>("aaa")) && (h1 == 0));
  DCHECK(ht.CreateHandle(h2, make_shared<string>("bbb")) && (h2 == 1));
  DCHECK(ht.CreateHandle(h3, make_shared<string>("ccc")) && (h3 == 2));
  DCHECK(ht.CloseHandle(h2));
  DCHECK(ht.CreateHandle(_h, make_shared<string>("xxx")) && (_h == 1)); // _h replaces h2
  DCHECK(ht.CreateHandle(_h, make_shared<string>("ccc")) && (_h == 3)); // 2 already used by "ccc"

  // now have 4 items, add more 4
  for (int i = 0; i < 4; i++) {
    DCHECK(ht.CreateHandle(_h, make_shared<string>("NNN")));
    cout << _h << "\n";
  }
  // now full
  DCHECK(!ht.CreateHandle(_h, make_shared<string>("404")));

  // let relax, close h1
  ht.CloseHandle(h1);
  // now can insert
  DCHECK(ht.CreateHandle(_h, make_shared<string>("jjj")));
}

void test_handle_table32_shptr_rw(TestInfo&) {
  HandleTable<unsigned, Shptr<string>> ht(8);
  unsigned h1, h2, h3, _h;
  DCHECK(ht.CreateHandle(h1, make_shared<string>("aaa")) && (h1 == 0));
  DCHECK(ht.CreateHandle(h2, make_shared<string>("bbb")) && (h2 == 1));
  DCHECK(ht.CreateHandle(h3, make_shared<string>("ccc")) && (h3 == 2));
  DCHECK(ht.CloseHandle(h2));
  DCHECK(ht.CreateHandle(_h, make_shared<string>("xxx")) && (_h == 1)); // _h replaces h2
  DCHECK(ht.CreateHandle(_h, make_shared<string>("ccc")) && (_h == 3)); // 2 already used by "ccc"

  // now have 4 items, add more 4
  for (int i = 0; i < 4; i++) {
    DCHECK(ht.CreateHandle(_h, make_shared<string>("NNN")));
    cout << _h << "\n";
  }
  // now full
  DCHECK(!ht.CreateHandle(_h, make_shared<string>("404")));

  // let relax, close h1
  ht.CloseHandle(h1);
  // now can insert
  DCHECK(ht.CreateHandle(_h, make_shared<string>("jjj")));

  // we weren't close h3, use it for test
  Shptr<string>* phdata = ht.FindHandleData(h3);
  DCHECK(phdata != nullptr);
  Shptr<string> hdata = *phdata;
  DCHECK(*hdata.get() == "ccc");
}
