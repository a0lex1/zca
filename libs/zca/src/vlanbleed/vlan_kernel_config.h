#pragma once

#include "./handle.h"

#include "co/base/bin_reader.h"
#include "co/base/bin_writer.h"

#include <sstream>

// Sides exchange VlanKernelConfig in Connect and ConnectResult msgs
struct VlanKernelConfig {

  // Fields
  vlhandle_t accept_max{ 0 };
  uint16_t queue_size{ 0 };
  uint32_t buffer_size{ 0 };

  // Serialization
  bool Serialize(co::BinWriter& writer) const;
  bool Unserialize(co::BinReader& reader);

  // Textualization
  std::string Textualize() const {
    std::stringstream ss;
    ss << "a:" << std::dec << (long)accept_max << ",q:" << queue_size << ",b:" << buffer_size;
    return ss.str();
  }

  // Operators
  bool operator==(const VlanKernelConfig& r) const {
    return queue_size == r.queue_size && buffer_size == r.buffer_size;
  }
  bool operator!=(const VlanKernelConfig& r) const {
    return !operator==(r);
  }
};

