#pragma once

#include "co/base/bin_reader.h"
#include "co/base/bin_writer.h"

namespace core {

struct CmdOpaqueData {
  std::string cmdline;
  std::string signature;

  bool Serialize(co::BinWriter& writer) {
    if (!writer.WriteString(cmdline)) {
      return false;
    }
    if (!writer.WriteString(signature)) {
      return false;
    }
    return true;
  }

  bool Unserialize(co::BinReader& rdr) {
    if (!rdr.ReadString(cmdline)) {
      return false;
    }
    if (!rdr.ReadString(signature)) {
      return false;
    }
    return true;
  }

};


}
