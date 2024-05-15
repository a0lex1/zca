#pragma once

#include "zca/core/back/backend_api.h"

#include "netshell/ns_cmd_result.h"
#include "netshell/netshell_error.h"

#include "co/base/interlocked_holder.h"
#include "co/xlog/define_logger_sink.h"

#include <atomic>

namespace modules {
namespace basecmd {
namespace back {

DECLARE_XLOGGER_SINK("basecmdbmdata", gZcaBasecmdBotModuleDataSink);

enum class CmdState {
  kNone = 0,
  kNewCommand = 1,
  kDoneWithoutPostpocess = 2,
  kPostprocessing = 3,
  kDoneWithPostprocess = 4,
  kNetworkError = 5,
  kUnserializeError = 6,
  kEnd = 7
};

// arrays match
static constexpr const char* cmdstate_names[]  = {
  "NONE",
  "NEW_CMD",
  "DONE_NO_POST",
  "POST_PROCING",
  "DONE_WITH_POST",
  "NET_ERR",
  "PROTO_ERR",
  "UNSER_ERR",
  nullptr
};
static constexpr size_t cmdstate_count = sizeof(cmdstate_names) / sizeof(cmdstate_names[0]);

// -----------------------------------------------------------------------------
// `bot-list` command displays these fields as columns (registered bot properties)
// -----------------------------------------------------------------------------
struct CmdInfo {
  CmdState                  state{ CmdState::kNone };
  std::string               bot_command;
  netshell::NsCmdResult     bot_result;
  std::string               post_command;
  netshell::NsCmdResult     post_result;
  Errcode                   net_error;
  netshell::NetshellError   netshell_error;
};

// -----------------------------------------------------------------------------

class BasecmdBotModuleData: public core::back::BackendBotModuleData
{
public:
  virtual ~BasecmdBotModuleData();

  BasecmdBotModuleData();

  bool AcquireCmdLock() {
    bool val = false;
    return cmdinfo_lock_.compare_exchange_strong(val, true, std::memory_order_acquire);
  }

  void ReleaseCmdLock() {
    cmdinfo_lock_.store(false, std::memory_order_release);
  }

  co::InterlockedHolder<CmdInfo>& GetCmdInfoInterlockedHolder() { return cmdinfo_holder_; }

  void StoreSalt(const std::string& new_salt) { salt_holder_.store(boost::make_shared<std::string>(new_salt)); }
  std::string LoadSalt() { return *salt_holder_.load(); }

private:
  std::atomic<bool> cmdinfo_lock_{false};
  co::InterlockedHolder<CmdInfo> cmdinfo_holder_;
  AtomicShptr<std::string> salt_holder_;
};

}}}

