#include "co/xlog/define_logger_sink.h"
#include "co/xlog/xlog.h"

using namespace std;

namespace co {
namespace xlog {

namespace detail {

void xlogVarTable::addSinkPtr(const char* name, Shptr<CLoggerSink>& shptr)
{
  shptrs_.insert(std::pair< std::string, Shptr<CLoggerSink>*>(name, &shptr));
}

SinkMap& xlogVarTable::getSinkPtrMap()
{
  return shptrs_;
}


}}}
