#pragma once

#include "zca/pki/salt_generator.h"
#include "zca/pki/signature_verifier.h"
#include "zca/pki/signature_creator.h"

#include "co/common.h"

namespace pki {

class PkiFactory {
public:
  virtual ~PkiFactory() = default;

  virtual Uptr<SaltGenerator> CreateSaltGenerator() = 0;
  virtual Uptr<SignatureVerifier> CreateSignatureVerifier() = 0;
  virtual Uptr<SignatureCreator> CreateSignatureCreator() = 0;
};

}


