#pragma once

#include <string>

namespace pki {

class SaltGenerator {
public:
  virtual ~SaltGenerator() = default;

  virtual void GenerateSalt(std::string& buffer) = 0;
};


}

