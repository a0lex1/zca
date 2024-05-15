#include "cc/bot_id.h"

#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace cc;
using namespace co;
using namespace std;

void test_cc_bot_id(TestInfo& ) {
  BotId d;

  d = BotId::FromUints(0, 0);
  syslog(_INFO) << "BotID $ " << d.ToStringRepr() << "\n";

  d = BotId::FromUints(1, 0);
  syslog(_INFO) << "BotID $ " << d.ToStringRepr() << "\n";

  d = BotId::FromUints(0xffffffff, 0xffffffff);
  syslog(_INFO) << "BotID $ " << d.ToStringRepr() << "\n";

  // too short (-1, -2)
  DCHECK(!d.FromStringRepr("000000000000000000000000000000"));
  DCHECK(!d.FromStringRepr("00000000000000000000000000000"));
  // too long
  DCHECK(!d.FromStringRepr(  "000000000000000000000000000000000"));


  DCHECK(d.FromStringRepr(   "00000000000000000000000000000001"));
  DCHECK(d.ToStringRepr() == "00000000000000000000000000000001");

  DCHECK(d.FromStringRepr("00000000000000030000000000000002"));
  auto xxx = d.ToStringRepr();
  DCHECK(d.ToStringRepr() == "00000000000000030000000000000002");

  DCHECK(d.FromStringRepr(   "f0000000c000000300000000000e0002"));
  DCHECK(d.ToStringRepr() == "f0000000c000000300000000000e0002");

}


