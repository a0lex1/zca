#include "netshell/ns_para_command_executor.h"

#include "co/async/wrap_post.h"

using namespace co;
using namespace co::async;

namespace netshell {

void NsParaCommandExecutor::ExecuteCommand(const std::string& ns_command,
  NsCmdResult& ns_result, HandlerWithNetshellErr handler)
{
  boost::asio::post(*strand_.get(),
    co::bind(&NsParaCommandExecutor::ExecuteCommandUnsafe, this,
      ns_command, std::ref(ns_result), handler));
}

void NsParaCommandExecutor::ExecuteCommandUnsafe(
  std::string ns_command,
  NsCmdResult& ns_result,
  HandlerWithNetshellErr handler)
{
 // DCHECK(op_map_.size() == 0);
  op_map_[cur_cmd_idx_] = ReadOp(ns_command, ns_result, handler);
  cur_cmd_idx_++;
  ns_cmd_writer_.AsyncWriteCommand(ns_command,
    wrap_post(*strand_.get(),
      co::bind(&NsParaCommandExecutor::HandleWriteCommand, this, _1, cur_cmd_idx_ - 1)));
  if (op_map_.size() == 1) {
    // We've had empty map before this operation; now 1 op; initiate read
    reading_ = true;
    InitiateRead();
  }
}

void NsParaCommandExecutor::HandleWriteCommand(NetshellError err, uint64_t cmd_idx) {
  auto it = op_map_.find(cmd_idx);
  DCHECK(it != op_map_.end());
  if (err) {
    // Error
    ReadOp read_op = it->second;
    op_map_.erase(it);
    read_op.user_handler(err);
    return;
  }
  // nothing to do, read may be initiated
}

void NsParaCommandExecutor::InitiateRead() {
  DCHECK(strand_->running_in_this_thread());
  ns_para_res_rdr_.AsyncReadParallelResult(cur_read_idx_, cur_read_result_,
   wrap_post(*strand_.get(),
     co::bind(&NsParaCommandExecutor::HandleReadResult, this, _1)));
}

void NsParaCommandExecutor::HandleReadResult(NetshellError err) {
  DCHECK(strand_->running_in_this_thread());
  reading_ = false;
  
  if (err) {
    // Error
    DropAllHandlersWithError(err);
    return;
  }

  auto it = op_map_.find(cur_read_idx_);

  if (it == op_map_.end()) {
    // Peer sent us an invalid command index; drop all existing handlers with error
    DropAllHandlersWithError(NetshellError(NetshellErrc::invalid_op_index,
      NetshellErrorInfo(cur_read_idx_)));
    return;
  }
  // Success
  ReadOp read_op = it->second;
  op_map_.erase(it);
  *read_op.ns_result = cur_read_result_;
  read_op.user_handler(NetshellError::NoError());

  if (op_map_.size() > 0 && !reading_) {
    reading_ = true;
    InitiateRead();
  }
}

void NsParaCommandExecutor::DropAllHandlersWithError(NetshellError err) {
  auto op_map_copy = op_map_;
  op_map_.clear();
  for (auto it = op_map_copy.begin(); it != op_map_copy.end(); it++) {
    it->second.user_handler(err);
  }
}

// ---

NsParaCommandExecutor::ReadOp::ReadOp(
  const std::string& _ns_command, NsCmdResult& _ns_result, HandlerWithNetshellErr _user_handler)
  :
  ns_command(_ns_command), ns_result(&_ns_result), user_handler(_user_handler)
{
}


}
