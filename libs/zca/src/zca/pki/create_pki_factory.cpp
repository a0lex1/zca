#include "zca/pki/create_pki_factory.h"
#include "zca/pki/dummy_pki_factory.h"

using namespace std;

namespace pki {

Uptr<PkiFactory> CreatePkiFactory(const string& name) {
  if (name == "dummy") {
    return make_unique<DummyPkiFactory>();
  }
  return nullptr;
}

}
