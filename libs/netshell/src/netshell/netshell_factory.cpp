#include "netshell/netshell_factory.h"
#include "netshell/textualizer.h"
#include "netshell/untextualizer.h"

#include "co/async/wrap_post.h"
#include "co/base/strings.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;

namespace netshell {

void NsCommandReaderText::AsyncReadCommand(string& cmd, HandlerWithErrcode handler) {
  line_rdr_.AsyncReadLine(cmd, handler);
}

void NsCommandReaderText::AsyncReadCommands(StringVector& cmd_list,
                                            HandlerWithErrcode handler) {
  line_rdr_.AsyncReadLines(cmd_list, handler);
}

// ----------------------

void NsCommandResultWriterText::AsyncWriteResult(const NsCmdResult& result,
                                                 HandlerWithErrcodeSize handler) {
  string serialized;
  NsCmdResultTextualizer texer(status_descriptors_, result);
  texer.Textualize(serialized);
  co::async::AsyncWriteAll(
    GetFiberStrandShptr(),
    line_writ_.GetStreamIo(),
    boost::asio::const_buffers_1(serialized.c_str(), serialized.length()),
    handler);
}

void NsCommandWriterText::AsyncWriteCommand(const string& cmd,
                                            HandlerWithNetshellErr handler) {
  // No need in strand, the lambda is [=] and the |handler| must be protected by calling code, not here
  line_writ_.AsyncWriteLine(
                 cmd,
                 [=](Errcode err, size_t unused_num_bytes) {
                              if (err) {
                                handler(NetshellError(NetshellErrc::stream_error,
                                        NetshellErrorInfo(err)));
                              }
                              else {
                                handler(NetshellError());
                              }
                            });
}

void NsCommandResultReaderText::AsyncReadResult(NsCmdResult& cmd_result,
                                                HandlerWithNetshellErr handler) {
  // Can be not in strand if this is the i/o initiation operation called from Start()
  DCHECK(user_read_result_ == nullptr);
  DCHECK(!user_read_handler_);
  user_read_result_ = &cmd_result;
  user_read_handler_ = handler;
  line_rdr_.AsyncReadLine(first_line_,
    wrap_post(GetFiberStrand(), co::bind(&NsCommandResultReaderText::HandleReadFirstLine, this, _1)));
}

void NsCommandResultReaderText::CleanupAbortedStop() {
  user_read_handler_ = nullptr;
}

void NsCommandResultReaderText::HandleReadFirstLine(Errcode err) {
  DCHECK(IsInsideFiberStrand());
  DCHECK(user_read_result_ != nullptr);
  DCHECK(user_read_handler_);
  user_read_result_->Clear();
  if (err) {
    PostAndClearReadHandler(NetshellError(NetshellErrc::stream_error, err));
    return;
  }
  NetshellError ns_err;

  // will be destroyed from PostAndClearReadHandler (TODO: rename PostAndClearReadHandler to Complete() or something like that)
  untexer_ = make_unique<NsCmdResultUntextualizer>(status_descriptors_, *user_read_result_);

  untexer_->UntextualizeFirstLine(first_line_, ns_err);
  if (!ns_err) {
    lines_left_ = user_read_result_->body_line_count;
    //first_line_.clear();
    if (lines_left_ > 0) {
      ReadBodyLines();
    }
    else {
      PostAndClearReadHandler(NetshellError::NoError());
    }
  }
  else {
    PostAndClearReadHandler(ns_err);
  }
}

void NsCommandResultReaderText::ReadBodyLines() {
  DCHECK(IsInsideFiberStrand());
  DCHECK(user_read_result_ != nullptr);
  DCHECK(user_read_handler_);
  line_rdr_.AsyncReadLine(more_line_,
        wrap_post(GetFiberStrand(),
              co::bind(&NsCommandResultReaderText::HandleReadBodyLine, this, _1)));
}

void NsCommandResultReaderText::HandleReadBodyLine(Errcode err) {
  DCHECK(IsInsideFiberStrand());
  DCHECK(user_read_result_ != nullptr);
  DCHECK(user_read_handler_);
  // TODO: why doesn't it go through |untexer_| ?
  if (err) {
    // total = 0 ??
    PostAndClearReadHandler(NetshellError(NetshellErrc::stream_error, err));
    return;
  }
  switch (user_read_result_->result_type) {
  case NsResultType::kNone: break; // line not saved
  case NsResultType::kMessage: break; // line not saved
  case NsResultType::kText:
    user_read_result_->text_lines.push_back(more_line_);
    break;
  case NsResultType::kCsv: {
    StringVector parts;
    string_split(more_line_, ",", parts);
    user_read_result_->csv_rows.push_back(parts);
    break;
  }
  case NsResultType::kSubresultArray:
    NOTREACHED();
  default:
    NOTREACHED();
  }

  --lines_left_;
  if (lines_left_ == 0) {
    PostAndClearReadHandler(NetshellError::NoError());
    return;
  }
  ReadBodyLines();
}

void NsCommandResultReaderText::PostAndClearReadHandler(const NetshellError& err) {
  auto hcopy(user_read_handler_);
  user_read_result_ = nullptr;
  user_read_handler_ = nullptr;
  untexer_ = nullptr;
  // No need in strand, handler is [=] (does not reference anything) and it's a calling code responsibility to guard the handler
  boost::asio::post(line_rdr_.GetStreamIo().GetIoContext(),
                    [hcopy, err]() {
                      hcopy(err);
                    });
}

// ----------------------

void NsParaCommandResultReaderText::AsyncReadParallelResult(uint64_t& cmd_index,
  NsCmdResult& cmd_result, HandlerWithNetshellErr handler)
{
  ThreadIdType tid = entered_from_tid_;
  if (tid != 0) {
    syslog(_TRACE) << "[tid " << (void*)ThreadId() << "]: Already entered from thread " << (void*)tid << "\n";
    NOTREACHED();
  }
  entered_from_tid_ = ThreadId();

  syslog(_TRACE) << "@@@@@@ AsyncReadParallelResult ENTERING (this=" << this << ")\n";
  DCHECK(handler);
  user_cmd_index_ = &cmd_index;
  user_handler_ = handler;
  user_cmd_result_ = &cmd_result;
  ns_rdr_text_.GetLineReader().AsyncReadLine(cmdindex_line_,
    wrap_post(ns_rdr_text_.GetFiberStrand(),
      co::bind(&NsParaCommandResultReaderText::HandleReadCmdindexLine,
        this, _1)));
  syslog(_TRACE) << "@@@@@@ AsyncReadParallelResult LEAVING (this=" << this << ")\n";

  entered_from_tid_ = 0;
}

void NsParaCommandResultReaderText::HandleReadCmdindexLine(Errcode err) {
  //syslog(_DBG) << "!!!!!! HandleReadCmdindexLine ENTER\n";
  DCHECK(ns_rdr_text_.GetFiberStrand().running_in_this_thread());
  DCHECK(user_handler_);
  if (err) {
    PostAndClearReadHandler(NetshellError(NetshellErrc::stream_error, NetshellErrorInfo(err)));
    return;
  }
  // Parse the line we've just read.
  if (!co::string_to_uint64(cmdindex_line_, *user_cmd_index_)) {
    PostAndClearReadHandler(NetshellError(NetshellErrc::cannot_parse_command_index_int,
      NetshellErrorInfo(cmdindex_line_)));
    return;
  }
  // We read line 0 which is the command index. Now rely on |ns_rdr_text_| to read
  // the ns result.
  ns_rdr_text_.AsyncReadResult(*user_cmd_result_, user_handler_);
  user_handler_ = nullptr;
  //user_handler_ = [] (NetshellError err) {};
  //syslog(_DBG) << "!!!!!! HandleReadCmdindexLine LEAVE\n";
}

void NsParaCommandResultReaderText::CleanupAbortedStop() {
  ns_rdr_text_.CleanupAbortedStop();
  user_handler_ = nullptr;
}

void NsParaCommandResultReaderText::PostAndClearReadHandler(const NetshellError& err) {
  DCHECK(user_handler_ != nullptr);
  auto hcopy(user_handler_);
  user_cmd_index_ = nullptr;
  user_handler_ = nullptr;
  // No need in strand, handler is [=] (does not reference anything) and it's a calling code responsibility to guard the handler
  boost::asio::post(ns_rdr_text_.GetLineReader().GetStreamIo().GetIoContext(),
    [hcopy, err]() {
      DCHECK(hcopy != nullptr);
      hcopy(err);
    });
}

// ------------------------------------------------------------------------------------------------

NsParaCommandResultWriterText::NsParaCommandResultWriterText(
  const NsStatusDescriptorTable& status_descriptors,
  Shptr<Strand> strand,
  co::async::StreamIo& stm_io)
  :
  Fibered(strand),
  line_writ_(stm_io), underlying_(status_descriptors, strand, stm_io)
{
}

void NsParaCommandResultWriterText::AsyncWriteParallelResult(uint64_t cmd_index,
  const NsCmdResult& cmd_result, HandlerWithErrcodeSize handler)
{
  // FIRST, write cmdindex line
  std::string cmdindex_line = std::to_string(cmd_index);
  line_writ_.AsyncWriteLine(cmdindex_line, wrap_post(GetFiberStrand(),
    co::bind(&NsParaCommandResultWriterText::HandleWriteCmdindexLine,
      this, _1, _2, boost::ref(cmd_result), handler)));
}

void NsParaCommandResultWriterText::HandleWriteCmdindexLine(
  Errcode err,
  size_t cmdindex_line_bytes,
  const NsCmdResult& cmd_result,
  HandlerWithErrcodeSize user_handler)
{
  // SECOND, write the netshell result
  if (err) {
    user_handler(err, 0);
    return;
  }
  // SUCCESS writing cmdindex line
  underlying_.AsyncWriteResult(cmd_result, wrap_post(GetFiberStrand(),
    co::bind(&NsParaCommandResultWriterText::HandleWriteResult,
      this, _1, _2, cmdindex_line_bytes, user_handler)));
}

void NsParaCommandResultWriterText::HandleWriteResult(Errcode err,
  size_t num_bytes,
  size_t cmdindex_line_bytes,
  HandlerWithErrcodeSize user_handler)
{
  user_handler(err, cmdindex_line_bytes + num_bytes);
}

void NsParaCommandResultWriterText::CleanupAbortedStop() {
  underlying_.CleanupAbortedStop();
}

// ------------------------------------------------------------------------------------------------

// Global
bool NsResultTypeByName(const string& type_name, NsResultType& t) {
  if (type_name == "NONE") {
    t = NsResultType::kNone;
    return true;
  }
  if (type_name == "MSG") {
    t = NsResultType::kMessage;
    return true;
  }
  if (type_name == "TEXT") {
    t = NsResultType::kText;
    return true;
  }
  if (type_name == "CSV") {
    t = NsResultType::kCsv;
    return true;
  }
  if (type_name == "RESULT_ARRAY") {
    t = NsResultType::kSubresultArray;
    return true;
  }
  return false;
}

string NetshellError::MakeErrorMessage() const
{
  switch (GetErrc()) {
  case NetshellErrc::ok: return DefaultErrcTitleOk();
  case NetshellErrc::stream_error: {
    stringstream ss;
    ss << GetErrcTitle() << " " << GetErrorInfo().stream_err_code;
    return ss.str();
  }
  case NetshellErrc::bad_part_count: return GetErrcTitle();
  case NetshellErrc::bad_status_code: return GetErrcTitle();
  case NetshellErrc::bad_ret_code: return GetErrcTitle();
  case NetshellErrc::bad_result_type: return GetErrcTitle();
  case NetshellErrc::bad_body_line_count: return GetErrcTitle();
  case NetshellErrc::unknown_status_code: return GetErrcTitle();
  case NetshellErrc::unexpected_line: return GetErrcTitle();
  case NetshellErrc::unexpected_end: return GetErrcTitle();
  case NetshellErrc::cannot_parse_command_index_int: {
    return GetErrcTitle() + string(" - ") + GetErrorInfo().bad_str;
  }
  case NetshellErrc::invalid_op_index: return GetErrcTitle() + string(" ") + string_from_uint64(GetErrorInfo().index64);
  default: return DefaultErrcTitleUnknown();
  }
}



}

