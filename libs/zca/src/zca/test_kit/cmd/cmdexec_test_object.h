#pragma once

#include "zca/test_kit/zca_test_object.h"

#include "zca/test_kit/cmd/cmdexec_test_object_params.h"

class CmdexecTestObject: public ZcaTestObjectSync {
public:
  virtual ~CmdexecTestObject() = default;
  
  using ZcaTestObjectSync::ZcaTestObjectSync;
  using ThreadModel = co::async::ThreadModel;
  using BotId = cc::BotId;

  CmdexecTestObject(ThreadModel& tm,
                    const CmdexecTestObjectParams& pams,
                    bool dry_run);

private:
  void EnableParties() override;
  void SetOptions() override;
  void AddModules() override;

  void OnSyncAllConnected() override;

private:
  CmdexecTestObjectParams pams_;
  bool dry_;
};

