#include "zca/netshell_status_descriptor_table.h"

#include "zca/modules/basecmd/back/cmdinfo_property_group.h"
#include "zca/modules/basecmd/back/basecmd_bot_module_data.h"

#include "netshell/textualizer.h"

#include "co/base/strings.h"

#include <sstream>

using namespace std;
using namespace netshell;

namespace modules {
namespace basecmd {
namespace back {

CmdinfoPropertyGroup::CmdinfoPropertyGroup(
  core::back::BackendLocalApi& backend_local_api,
  core::back::BackendGlobalApi& backend_global_api)
  :
  backend_local_api_(backend_local_api),
  backend_global_api_(backend_global_api)
{

}

static const char* const skip_tag = "-";

// TODO: WARNING, max_line_length and max_line_count are used only for cmd result, other fields not affected
void CmdinfoPropertyGroup::ReadForBot(
  Shptr<cc::ICcBot> bot, StringVector& props,
  size_t max_line_length,
  size_t max_line_count)
{
  // inside any fiber!
  props.clear();

  BasecmdBotModuleData* our_bot_module_data;
  our_bot_module_data =
    backend_local_api_.GetBotModuleDataAs<BasecmdBotModuleData>(
      *bot.get());
  DCHECK(our_bot_module_data);

  // When bot module data is set to bot, it already contains stored pointer.
  CmdInfo cmdinfo;
  bool loaded = our_bot_module_data->GetCmdInfoInterlockedHolder().LoadData(cmdinfo);
  DCHECK(loaded);

  // Be liberalistic here. Let malformed values go. We need
  // it to find more bugz with stress tests. Replace bad values with
  // descriptive strings.

  // stcode is useless for showing
  /*props.emplace_back(co::string_printf("%d", (int)cmdinfo.state));
  if (props.back().length() > max_line_length) {
    props.back().resize(max_line_length);
  }*/

  if ((int)cmdinfo.state < cmdstate_count) {
    const char* const statename = cmdstate_names[(int)cmdinfo.state];
    props.emplace_back(statename);
    if (props.back().length() > max_line_length) {
      props.back().resize(max_line_length);
    }
  }
  else {
    props.emplace_back("<bad-state>");
  }

  //
  props.emplace_back(cmdinfo.bot_command);
  if (props.back().length() > max_line_length) {
    props.back().resize(max_line_length);
  }

  // now add bot_command_result
  std::string str_bot_result;
  if (cmdinfo.bot_result.status_code == kNsCmdNoResult) {
    // Don't show empty results, shortcut'em all
    str_bot_result = skip_tag;
  }
  else {
    NsCmdResultTextualizer texer(gZcaNsStatusDescriptorTable, cmdinfo.bot_result);
    texer.TextualizeToString(str_bot_result, max_line_length, max_line_count);
  }
  //
  props.emplace_back(str_bot_result);
  // was already cut to max length in texer.TextualizeToString
  
  props.emplace_back(cmdinfo.post_command);
  if (props.back().length() > max_line_length) {
    props.back().resize(max_line_length);
  }


  // *******************************************************************
  // Netshell results can't have bad state, right?
  // I hope it can't. Otherwise the bot will send us malicious result
  // *******************************************************************
  std::string str_post_result;
  if (cmdinfo.post_result.status_code == kNsCmdNoResult) {
    // Don't show empty results, shortcut'em all
    str_post_result = skip_tag;
  }
  else {
    NsCmdResultTextualizer texer(gZcaNsStatusDescriptorTable, cmdinfo.post_result);
    texer.TextualizeToString(str_post_result, max_line_length, max_line_count);
  }
  //
  props.emplace_back(str_post_result);

  //
  stringstream ss;
  if (!cmdinfo.net_error) {
    // Don't show empty errors
    ss << skip_tag;
  }
  else {
    ss << cmdinfo.net_error;
  }
  props.emplace_back(ss.str());
  if (props.back().length() > max_line_length) {
    props.back().resize(max_line_length);
  }

  //
  ss.clear();
  if (!cmdinfo.netshell_error) {
    // Don't show empty errors
    ss << skip_tag;
  }
  else {
    ss << cmdinfo.netshell_error;
  }
  props.emplace_back(ss.str());
  if (props.back().length() > max_line_length) {
    props.back().resize(max_line_length);
  }


  props.emplace_back(our_bot_module_data->LoadSalt());
  if (props.back().length() > max_line_length) {
    props.back().resize(max_line_length);
  }
}

}}}
