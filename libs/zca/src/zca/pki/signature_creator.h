#pragma once

namespace pki {

class SignatureCreator {
public:
  virtual ~SignatureCreator() = default;

  virtual void CreateSignature(const void*, size_t, std::string& signature) = 0;
};

}
