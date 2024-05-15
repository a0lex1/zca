#include "zca/modules/basecmd/back/basecmd_backend_module.h"

#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;
using namespace net;

#define llog() Log(_DBG) << "BasecmdBotModDat " << SPTR(this) << " "

namespace modules {
namespace basecmd {
namespace back {

DEFINE_XLOGGER_SINK("basecmdbmdata", gZcaBasecmdBotModuleDataSink);
#define XLOG_CURRENT_SINK gZcaBasecmdBotModuleDataSink

BasecmdBotModuleData::~BasecmdBotModuleData() {
  llog() << "~~~DTOR~~~\n";
}

BasecmdBotModuleData::BasecmdBotModuleData() {
  llog() << "CTOR\n";
}

}}}


