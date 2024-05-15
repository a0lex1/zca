#pragma once

#include "netshell/textualizer.h"

#include "co/base/bin_writer.h"

namespace netshell {

// New variant of serialization/unserialization; don't go through the textualization
class NsCmdResultSerializer {
public:
  NsCmdResultSerializer(const NsStatusDescriptorTable& status_descriptors,
    const NsCmdResult& ns_result);
  
  bool Serialize(co::BinWriter& writer);

private:
  NsCmdResultTextualizer ns_texer_;
};

}
