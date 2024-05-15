#pragma once

#include "netshell/serializer.h"
#include "co/base/bin_writer.h"

using namespace std;

namespace netshell {

static std::string GetSerializedNsCmdResult(const NsStatusDescriptorTable& sdt, const NsCmdResult& res) {
  NsCmdResultSerializer serer(sdt, res);
  string buf;
  co::BinWriter writer(buf);
  serer.Serialize(writer);
  return buf;
}


}
