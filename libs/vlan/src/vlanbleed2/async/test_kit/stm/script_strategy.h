#pragma once

#include "vlanbleed2/async/test_kit/stm/strategy.h"
#include "vlanbleed2/async/test_kit/stm/script.h"

//
// Script Strategy (a Strategy, implemented by a Script)
//

class ScriptStrategy : public Strategy {
 public:
  virtual ~ScriptStrategy() = default;

  ScriptStrategy(const Script& script, bool dry_run)
    : script_(script), dry_run_(dry_run)
  {
  }

  // Allow user to set stream after CTOR. Adding Get counterpart seems reasonable.
  using Strategy::GetStrategyContext;

  virtual void Start(co::RefTracker rt) = 0; //emphasize

  const Script& GetScript() const { return script_; }
  std::vector<SOpRes>& GetOpResultVector() { return op_results_; }

 protected:
  //const Script& GetScript() const { return script_; }
  bool GetNextOp(SOp& op) {
    if (cur_op_idx_ == script_.GetOps().size()) {
      // end of script
      return false;
    }
    op = script_.GetOps()[cur_op_idx_];
    cur_op_idx_ += 1;
    return true;
  }
  bool IsDryRun() const { return dry_run_; }

 private:
  Script script_;
  size_t cur_op_idx_{ 0 };
  bool dry_run_;
  std::vector<SOpRes> op_results_;
};


