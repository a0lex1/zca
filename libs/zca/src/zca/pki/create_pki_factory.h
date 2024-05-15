#pragma once

#include "zca/pki/pki_factory.h"

namespace pki {

Uptr<PkiFactory> CreatePkiFactory(const std::string& name);

}

