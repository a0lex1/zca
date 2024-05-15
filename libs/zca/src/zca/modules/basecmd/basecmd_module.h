#pragma once

#include "zca/engine/sdk/module.h"

namespace modules {
namespace basecmd {

class BasecmdModule : public engine::Module {
public:
  virtual ~BasecmdModule() = default;

  using DispatchContext = engine::DispatchContext;
  using DispatchCmdData = engine::DispatchCmdData;
  using DispatchCmdTask = engine::DispatchCmdTask;

  BasecmdModule();

private:
  void DispatchEchoargsCmd(DispatchContext&, DispatchCmdData&, Shptr<Strand> strand, Shptr<DispatchCmdTask>& new_task);
  void DispatchEchoinputCmd(DispatchContext&, DispatchCmdData&, Shptr<Strand> strand, Shptr<DispatchCmdTask>& new_task);
  void DispatchWaitCmd(DispatchContext&, DispatchCmdData&, Shptr<Strand> strand, Shptr<DispatchCmdTask>& new_task);

  const engine::DispatchCmdFuncMap& GetDispatchCmdFuncMap() override {
    return disp_table_.GetMap();
  }
  
protected:
  engine::DispatchTable disp_table_;
};

}}

