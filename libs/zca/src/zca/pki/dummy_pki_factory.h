#pragma once

#include "zca/pki/pki_factory.h"

#include "co/base/base64.h"
#include "co/base/crc32.h"
#include "co/base/strings.h"

#include "co/xlog/xlog.h"

using namespace std;
using namespace co;

namespace pki {

namespace dummy {

class DummySaltGenerator: public SaltGenerator {
public:
  virtual ~DummySaltGenerator() = default;

  void GenerateSalt(string& buffer) override {
    syslog(_DBG) << "GenerateSalt called\n";
    buffer = co::encode64(string_printf("%d", counter_), false);
    counter_ += 1;
  }

private:
  uint64_t counter_{0};
};

class DummySignatureVerifier: public SignatureVerifier {
public:
  virtual ~DummySignatureVerifier() = default;

  void VerifySignature(const std::string& buffer, const std::string& signature, bool& valid) {
    syslog(_DBG) << "VerifySignature() called\n";
    if (signature.length() == 4) {
      unsigned* puint = (unsigned*)signature.c_str();
      unsigned crc = co::crc32(-1, buffer.c_str(), buffer.length());
      if (crc == *puint) {
        valid = true;
        return;
      }
    }
    valid = false;
    return;
  }
};

class DummySignatureCreator : public SignatureCreator {
public:
  virtual ~DummySignatureCreator() = default;

  void CreateSignature(const void* buf, size_t buf_len, std::string& signature) override {
    unsigned crc = co::crc32(-1, buf, buf_len);
    signature = std::string((char*)&crc, 4);
  }
};

}

class DummyPkiFactory: public PkiFactory {
public:
  virtual ~DummyPkiFactory() = default;

  Uptr<SaltGenerator> CreateSaltGenerator() override {
    return make_unique<dummy::DummySaltGenerator>();
  }
  Uptr<SignatureVerifier> CreateSignatureVerifier() override {
    return make_unique<dummy::DummySignatureVerifier>();
  }
  Uptr<SignatureCreator> CreateSignatureCreator() {
    return make_unique<dummy::DummySignatureCreator>();
  }
};

}

