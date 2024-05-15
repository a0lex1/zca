#pragma once

#include "zca/engine/sdk/engine_session.h"
#include "zca/pki/signature_creator.h"

#include "netshell/ns_para_command_executor.h"

namespace core {
namespace front {

class AdminSessionCustomApi : public engine::CustomApi {
public:
  virtual ~AdminSessionCustomApi() = default;

  virtual netshell::NsParaCommandExecutor& GetBackshellParaExecutor() = 0;
  virtual pki::SignatureCreator& GetSignatureCreator() = 0;
};

}}
