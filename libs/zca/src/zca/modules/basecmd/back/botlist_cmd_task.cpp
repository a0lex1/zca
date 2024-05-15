#include "zca/modules/basecmd/back/botlist_cmd_task.h"
#include "zca/core/apply_removal_indices.h"
#include "zca/core/res_with_msg_body.h"

#include "zca/netshell_status_descriptor_table.h"

#include "co/base/cmdline/parse_cmdline_to_argv.h"
#include "co/base/cmdline/getopt_cpp.h"
#include "co/base/csv/csv_to_tabulate.h"

using namespace std;
using namespace co;
using namespace co::cmdline;
using namespace netshell;

namespace modules {
namespace basecmd {
namespace back {

BotlistCmdTask::BotlistCmdTask(Shptr<Strand> strand)
  :
  DispatchCmdTask(strand)
{
}

void BotlistCmdTask::BeginIo(RefTracker rt) {
  rt.SetReferencedObject(shared_from_this());

  StringVector args;
  if (!cmdline::ParseCmdlineToArgv(GetNsCommand().c_str(), args)) {
    SetResult(core::ResWithMsgBody(-1, "Cannot parse cmdline"));
    return;
  }


  Getopt go;
  int getopt_ret;
  while ((getopt_ret = go.Execute(args, "ncw:h:i:e:a:b:")) != -1) {
    switch (getopt_ret) {
    case 'n':
      no_header_ = true;
      break;
    case 'c':
      csv_ = true;
      break;
    case 'w':
      max_line_length_ = atoi(go.OptArg()); // if NaN, 0
      break;
    case 'h':
      max_line_count_ = atoi(go.OptArg()); // if NaN, 0
      break;
    case 'i':
      if (!bot_filter_.AddWhitelist(go.OptArg())) {
        SetResult(core::ResWithMsgBody(-1, "Cannot parse include filter"));
        return;
      }
      break;
    case 'e':
      if (!bot_filter_.AddBlacklist(go.OptArg())) {
        SetResult(core::ResWithMsgBody(-1, "Cannot parse exclude filter"));
        return;
      }
      break;
    case 'a':
      field_filter_.AddInclude(go.OptArg());
      break;
    case 'b':
      field_filter_.AddExclude(go.OptArg());
      break;
    case '?':
      SetResult(core::ResWithMsgBody(-1, "Ambiguous parameter"));
      return;
    default:
      NOTREACHED();
    }
  }

  GetModule().GetGlobalApiAs<core::back::BackendGlobalApi>()
      .GetCcServer().ExecuteBotListAccessCallback(
        co::bind(&BotlistCmdTask::HandleExecuteBotListAccessCallback,
          shared_from_this(),
          _1, rt));
}

void BotlistCmdTask::HandleExecuteBotListAccessCallback(cc::ICcBotList& bot_list,
                                                        RefTracker rt)
{
  // inside botlist fiber
  using BotPropertyGroup = core::back::BotPropertyGroup;
  using BotPropertyGroupSet = core::back::BotPropertyGroupSet;

  const bool print_header = true;

  BotPropertyGroupSet& bot_propg_set(
    GetModule()
    .GetGlobalApiAs<core::back::BackendGlobalApi>()
    .GetBotPropertyGroupSet());
  size_t grpcount = bot_propg_set.GetBotPropertyGroupCount();

  if (!csv_) {
    // If tabulate pretty print mode, don't allow no_header
    no_header_ = false;
  }

  vector<StringVector> csv_rows;

  // To create header for entire csv table, let default properties go first
  StringVector header = { "bid", "ip", "ver" };

  // Then add extra properties as columns
  for (size_t ngrp = 0; ngrp < grpcount; ngrp++) {
    core::back::BotPropertyGroup& grp(bot_propg_set.GetBotPropertyGroup(ngrp));
    size_t propcount(grp.GetPropertyCount());
    StringVector propnames;

    grp.GetPropertyNames(propnames);
    DCHECK(propcount == propnames.size());

    header.insert(header.end(), propnames.begin(), propnames.end());
  }

  // Table Header
  if (!no_header_) {
    // Add header to output csv buffer
    csv_rows.push_back(header);
  }

  // Removal indices - once computed, than used

  field_filter_.SetHeaderRow(header);

  vector<size_t> removal_indices;
  field_filter_.CreateRemovalIndices(removal_indices);

  // Apply to header
  if (!no_header_) {
    core::ApplyRemovalIndices(csv_rows.back(), removal_indices);
  }

  bot_filter_.SetHeaderRow(header); // header contains initial columns before applying removal indices

  // CSV Body: first collect botlist as csv at place, then convert to tabulate if needed
  size_t position = 0;
  for (const Shptr<cc::ICcBot>& bot : bot_list)
  {
    cc::ICcBotHandshakeData* hsdata = bot->GetReadonlyData().GetHandshakeData();
    if (hsdata == nullptr) {
      // ---------------------------------------------
      // Don't show bots that are not yet handshaked
      // Don't even pass them to filter
      // ---------------------------------------------
      continue;
    }
    cc::BotId bot_id(hsdata->GetBotId());
    cc::BotVersion bot_ver(hsdata->GetBotVersion());

    auto s_ipaddr(bot->GetReadonlyData().GetRemoteAddress().ToString());
    auto s_bot_id(bot_id.ToStringRepr());
    auto s_bot_ver(string_printf("%d.%d", CO_HIWORD(bot_ver), CO_LOWORD(bot_ver)));
    DCHECK(sizeof(bot_ver) == sizeof(uint32_t));

    // Form csv row for this bot, default properties first
    StringVector csv_row{ s_bot_id, s_ipaddr, s_bot_ver };

    // Append extra properties
    for (size_t ngrp = 0; ngrp < grpcount; ngrp++) {
      BotPropertyGroup& grp(bot_propg_set.GetBotPropertyGroup(ngrp));

      // Skip grp.GetGroupName(), no room to show
      StringVector prop_names;
      grp.GetPropertyNames(prop_names);

      StringVector prop_values;
      grp.ReadForBot(bot, prop_values, max_line_length_, max_line_count_); // interlocked

      // add values to row
      csv_row.insert(csv_row.end(), prop_values.begin(), prop_values.end());
    }
    
    // First filter, then apply removal indices
    if (bot_filter_.ApplyFilterToRow(csv_row)) {
      // Add row to output csv buffer
      csv_rows.push_back(csv_row);
      core::ApplyRemovalIndices(csv_rows.back(), removal_indices);
    }

    position += 1;
  }


  // Convert to tabulate if no -c flag specified
  if (!csv_) {
    // Text output
    string txt;
    if (csv_rows.empty()) {
      // tabulate crashes if you pass em empty result
      // Need to control this ourselves
      DCHECK(txt == "");
    }
    else {
      co::csv::CsvToTabulateDump(csv_rows, txt);
    }
    // It was CSV, now entire result is recreated as text result
    SetResult(NsCmdResult(kNsCmdExecuted, 0, NsResultType::kText)
      .WithTextBody(txt));
  }
  else {
    // CSV output
    SetResult(NsCmdResult(kNsCmdExecuted, 0, NsResultType::kCsv)
      .WithCsvBody(csv_rows));
  }
}

void BotlistCmdTask::StopUnsafeExtra()
{
  // No way to stop task, pretend we've sent our magic signal
  // SendMagicSignal();
}


}}}
