#include "zca/modules/basecmd/front/botlist_cmd_task.h"

#include "zca/core/front/admin_session_custom_api.h"
#include "zca/core/res_with_msg_body.h"

#include "zca/netshell_status_descriptor_table.h"

#include "co/base/cmdline/parse_cmdline_to_argv.h"
#include "co/base/cmdline/getopt_cpp.h"
#include "co/base/csv/csv_to_tabulate.h"
#include "co/base/csv/escape_unescape.h"

using namespace std;
using namespace co;
using namespace co::cmdline;
using namespace netshell;

namespace modules {
namespace basecmd {
namespace front {

BotlistCmdTask::BotlistCmdTask(Shptr<Strand> strand)
  : DispatchCmdTask(strand)
{
}

void BotlistCmdTask::BeginIo(RefTracker rt) {
  rt.SetReferencedObject(shared_from_this());
 
  StringVector args;
  if (!cmdline::ParseCmdlineToArgv(GetNsCommand().c_str(), args)) {
    SetResult(core::ResWithMsgBody(-1, "Cannot parse cmdline"));
    return;
  }

  Uptr<size_t> f_width;
  Uptr<size_t> f_height;
  vector<string> f_whitelist;
  vector<string> f_blacklist;
  vector<string> f_include;
  vector<string> f_exclude;

  Getopt go;
  int getopt_ret;
  while ((getopt_ret = go.Execute(args, "ncw:h:i:e:a:b:")) != -1) {
    switch (getopt_ret) {
    case 'n':
      f_no_csv_header_ = true;
      break;
    case 'c':
      f_csv_ = true;
      break;
    case 'w':
      f_width = make_unique<size_t>(atoi(go.OptArg()));
      break;
    case 'h':
      f_height = make_unique<size_t>(atoi(go.OptArg()));
      break;
    case 'i':
      f_whitelist.push_back(go.OptArg());
      break;
    case 'e':
      f_blacklist.push_back(go.OptArg());
      break;
    case 'a':
      f_include.push_back(go.OptArg());
      break;
    case 'b':
      f_exclude.push_back(go.OptArg());
      break;
    case '?':
      SetResult(core::ResWithMsgBody(-1, "Ambiguous parameter"));
      return;
    default:
      NOTREACHED();
    }
  }

  string back_cmdline = "bot-list -c "; // CSV with header

  if (f_width) {
    back_cmdline += string_printf("-w %d ", *f_width);
  }
  if (f_height) {
    back_cmdline += string_printf("-h %d ", *f_height);
  }
  for (size_t i = 0; i < f_whitelist.size(); i++) {
    back_cmdline += "-i " + f_whitelist[i] + " ";
  }
  for (size_t i = 0; i < f_blacklist.size(); i++) {
    back_cmdline += "-e " + f_blacklist[i] + " ";
  }
  for (size_t i = 0; i < f_include.size(); i++) {
    back_cmdline += "-a " + f_include[i] + " ";
  }
  for (size_t i = 0; i < f_exclude.size(); i++) {
    back_cmdline += "-b " + f_exclude[i] + " ";
  }

  auto& para_executor(GetCustomApiAs<core::front::AdminSessionCustomApi>()->GetBackshellParaExecutor());

  para_executor.ExecuteCommand(back_cmdline, back_result_,
    wrap_post(GetFiberStrand(), co::bind(&BotlistCmdTask::HandleExecBackCmdline,
      shared_from_this(), _1, rt)));
}

void BotlistCmdTask::HandleExecBackCmdline(netshell::NetshellError ns_err, RefTracker rt) {
  if (ns_err) {
    SetResult(core::ResWithMsgBody(
      -1, string_printf("backend returned ns_err: %s", ns_err.MakeErrorMessage().c_str())));
    return;
  }
  if (back_result_.result_type != NsResultType::kCsv) {
    SetResult(core::ResWithMsgBody(
      -1, string_printf("backend returned unexpected res type: %d", back_result_.result_type)));
    return;
  }
  
  // TODO: append columns
  //for (size_t i = 0; i < back_result_.csv_rows.size(); i++) {//demo
  //  back_result_.csv_rows[i].push_back("XXX");
  //}

  if (!f_csv_) {
    // Return tabulate text
    string txt;
    if (back_result_.csv_rows.empty()) {
      DCHECK(txt == "");
    }
    else {
      // We got escaped data (we used bot-list -c command) from backend. Unescape.
      for (size_t i = 0; i < back_result_.csv_rows.size(); i++) {
        for (size_t j = 0; j < back_result_.csv_rows[i].size(); j++) {
          back_result_.csv_rows[i][j] = co::csv::CsvUnescape(back_result_.csv_rows[i][j]);
        }
      }
      co::csv::CsvToTabulateDump(back_result_.csv_rows, txt);
    }
    SetResult(NsCmdResult(kNsCmdExecuted, 0, NsResultType::kText)
      .WithTextBody(txt));
  }
  else {
    if (f_no_csv_header_) {
      back_result_.csv_rows.erase(back_result_.csv_rows.begin());
    }
    SetResult(back_result_);
  }
}

void BotlistCmdTask::StopUnsafeExtra() {
}

}}}
