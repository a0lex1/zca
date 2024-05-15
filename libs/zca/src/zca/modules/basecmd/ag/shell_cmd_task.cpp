#include "zca/modules/basecmd/ag/shell_cmd_task.h"
#include "zca/netshell_status_descriptor_table.h"

#include "co/async/wrap_post.h"

#include "co/base/cmdline/parse_cmdline_to_argv.h"
#include "co/base/cmdline/join_cmd_line.h"
#include "co/base/cmdline/getopt_cpp.h"

using namespace std;
using namespace co;
using namespace co::cmdline;
using namespace netshell;

namespace modules {
namespace basecmd {
namespace ag {

ShellCmdTask::ShellCmdTask(Shptr<Strand> strand)
  :
  DispatchCmdTask(strand), redirector_(strand->context()),
  timer_(strand->context())
{
}

void ShellCmdTask::BeginIo(RefTracker rt) {
  rt.SetReferencedObject(shared_from_this());

  // Algorithm here:
  // Parse cmdline, get optional opts, then join the rest and pass it to Exec
  //
  StringVector args;
  if (!cmdline::ParseCmdlineToArgv(GetNsCommand().c_str(), args)) {
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
      .WithMessageBody("Cannot parse cmdline"));
    return;
  }

  Getopt getopt_cpp;
  int getopt_ret;
  while ((getopt_ret = getopt_cpp.Execute(args, "t:")) != -1) {
    switch (getopt_ret) {
    case 't':
      timeout_secs_ = atoi(getopt_cpp.OptArg());
      if (!timeout_secs_) {
        SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
          .WithMessageBody(string_printf("Invalid timeout - %s", getopt_cpp.OptArg())));
        return;
      }
      break;
    default:
      NOTREACHED();
    }
  }
  string joined_cmdline;
  joined_cmdline = JoinCmdLine(args, getopt_cpp.OptInd());

  Errcode err;
  redirector_.Exec(joined_cmdline, err);

  if (err) {
    SetResult(NsCmdResult(kNsCmdExecuted, -1, NsResultType::kMessage)
      .WithMessageBody(string_printf("Exec failed - err %s", err.message().c_str())));
    return;
  }

  // If needed, setup the timer
  //
  if (timeout_secs_ != -1) {
    timer_.expires_from_now(boost::posix_time::seconds(timeout_secs_));
    timer_.async_wait(wrap_post(GetFiberStrand(), co::bind(&ShellCmdTask::HandleTimeout,
      shared_from_this(), _1, rt)));
  }

  // Now read program's output, bufferize it and, when program terminates, return it
  //
  ReadOutputAgain(rt);
}

void ShellCmdTask::HandleTimeout(Errcode err, RefTracker rt) {
  if (err) {
  }
  redirector_.Terminate(-2);
}

void ShellCmdTask::ReadOutputAgain(RefTracker rt) {
  redirector_.AsyncReadSome(boost::asio::mutable_buffers_1(
    portion_buffer_, sizeof(portion_buffer_)),
    wrap_post(GetFiberStrand(), co::bind(&ShellCmdTask::HandleReadOutput,
      shared_from_this(), _1, _2, rt)));

}

void ShellCmdTask::FixUnprintableChars() {
  for (size_t i = 0; i < final_buffer_.length(); i++) {
    // Escape all unprintable, but not \r and \n chars
    unsigned char c = final_buffer_[i];
    if (c >= 128) {
      final_buffer_[i] = '.';
    }
  }
}

void ShellCmdTask::HandleReadOutput(Errcode err, size_t num_bytes, RefTracker rt) {
  if (err || !num_bytes) {

    FixUnprintableChars();

    SetResult(NsCmdResult(kNsCmdExecuted, 0, NsResultType::kText)
      .WithTextBody(final_buffer_));
  }
  else {
    // Read more
    final_buffer_.append(portion_buffer_, num_bytes);
    ReadOutputAgain(rt);
  }
}

void ShellCmdTask::StopUnsafeExtra() {
  redirector_.Terminate(-1);
}

}}}
