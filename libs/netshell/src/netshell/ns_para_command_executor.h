#pragma once

#include "netshell/netshell_factory.h"

namespace netshell {

class NsParaCommandExecutor {
public:
  NsParaCommandExecutor(
    NsCommandWriter& ns_cmd_writer, NsParaCommandResultReader& ns_para_res_rdr,
    Shptr<Strand> strand)
    :
    ns_cmd_writer_(ns_cmd_writer), ns_para_res_rdr_(ns_para_res_rdr),
    strand_(strand)
  {
  }
  
  // |handler| is called directly, so caller should be wrapped to post to eliminate the endless recursion
  void ExecuteCommand(const std::string& ns_command, NsCmdResult& ns_result, HandlerWithNetshellErr handler);
  
private:
  void ExecuteCommandUnsafe(std::string, NsCmdResult&, HandlerWithNetshellErr);

  struct ReadOp {
    std::string ns_command;
    NsCmdResult* ns_result{ nullptr };
    HandlerWithNetshellErr user_handler;

    ReadOp() {}
    ReadOp(const std::string&, NsCmdResult&, HandlerWithNetshellErr);
  };
  
  void HandleWriteCommand(NetshellError, uint64_t);
  void InitiateRead();
  void HandleReadResult(NetshellError);
  void DropAllHandlersWithError(NetshellError);
  
private:
  Shptr<Strand> strand_;
  uint64_t cur_cmd_idx_{ 0 };
  NsCommandWriter& ns_cmd_writer_;
  NsParaCommandResultReader& ns_para_res_rdr_;
  std::map<uint64_t, ReadOp> op_map_;
  bool reading_{ false };
  uint64_t cur_read_idx_;
  NsCmdResult cur_read_result_;
};

}

