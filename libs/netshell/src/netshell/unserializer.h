#pragma once

#include "netshell/untextualizer.h"
#include "co/base/bin_reader.h"

namespace netshell {

class NsCmdResultUnserializer {
public:
  NsCmdResultUnserializer(const NsStatusDescriptorTable& status_descriptors,
    NsCmdResult& ns_result);
  
  void Unserialize(co::BinReader& reader, NetshellError& err);

private:
  NsCmdResultUntextualizer untexer_;
};

}

