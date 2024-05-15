#pragma once

namespace pki {


class SignatureVerifier {
public:
  virtual ~SignatureVerifier() = default;

  //virtual void SetVerificationContext(SignatureVerificationContext&) = 0;

  virtual void VerifySignature(const std::string& buffer, const std::string& signature, bool& valid) = 0;
};

}
