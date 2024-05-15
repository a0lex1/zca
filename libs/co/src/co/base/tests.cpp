#include "co/base/tests.h"
#include "co/base/cmdline/keyed_cmdline.h"
#include "co/base/config.h"
#include "co/base/print_diagnostic_information.h"

#include "co/xlog/configs.h"

#include <iostream>

using namespace std;
using namespace co::cmdline;
using namespace co::xlog;
using namespace co::xlog::configs;

namespace co {
  
static LogConfigFromDictNoexcept log_conf;

int RunTests(
  cmdline::KeyedCmdLine<char>& cmd_line,
  const vector<pair<string, co::TestTable>>& tables,
  bool need_only_one/*=false*/)
{
  // Build |all_tests| vector by merging tests from all
  // tables (using |table_order| if specified, e.g. if not empty)
  vector<TestTableEntry> all_tests;

  for (const auto& it : tables) {
    const string& table_name(it.first);
    const TestTable& table(it.second);
    for (const TestTableEntry& entry : table) {
      all_tests.push_back(entry);
    }
  }

  TestInfo info;
  info.opts_dict = cmd_line.GetNamedArgs();

  // User may just want to see the test list.
  if (KEY_FOUND(info.opts_dict, "view-list")) {
    cout << "^^^ Showing test table ^^^\n";
    for (auto& test_entry: all_tests) {
      cout << "[+] " << test_entry.func_name << "\n";
    }
    cout << "\n";
    cout << "Select one of them or right all\n";
    return 1;
  }

  // No, user wants something else.
  StringVector& uargs = cmd_line.GetUnnamedArgs();

  if (need_only_one) {
    if (uargs.size() != 1) {
      cout << "*** Need exactly one test name in cmd_line\n";
      return -2;
    }
  }

  auto found_in_cmd_line = [&](const char* name) {
    for (size_t i = 0; i < uargs.size(); i++) {
      if (!strcmp(uargs[i].c_str(), name)) {
        return true;
      }
    }
    return false;
  };
  auto found_in_table = [&](const char* name) {
    for (size_t i = 0; i < all_tests.size(); i++) {
      if (!strcmp(all_tests[i].func_name, name)) {
        return true;
      }
    }
    return false;
  };

  bool test_repeat_found;
  uint32_t test_repeat_val = 1;
  OverrideFromDict<string, string, decltype(test_repeat_val)>(
    info.opts_dict,
    "test-repeat",
    test_repeat_val,
    ConsumeAction::kConsume,
    Necessity::kOptional,
    &test_repeat_found);

  syslog(_INFO) << "test_repeat => " << test_repeat_val << "\n";

  // Black list. Minus sign blacklists the test. Example:
  // test_co.exe --log-sevs=*:debug -test_co_async_text_socket_fragmented -test_co_xlog
  StringVector blacklist;
  size_t old_uargs_count = uargs.size();
  for (StringVector::iterator it = uargs.begin();
       it != uargs.end()
       ;)
  {
    auto test_name(*it);
    if (test_name.length() && test_name[0] == '-') {
      test_name = test_name.substr(1);      
      if (!found_in_table(test_name.c_str())) {
        cout << "****** ERROR! -blacklisting non-existing test - " << test_name << "\n";
        return -3;
      }

      // This is now blacklisted.
      blacklist.push_back(test_name);
      it = uargs.erase(it);
    }
    else {
      it++;
    }
  }
  DCHECK(uargs.size() == old_uargs_count - blacklist.size());

  if (blacklist.size()) {
    cout << "%%%%%%%%%%%%%%%% BLACKLIST: %%%%%%%%%%%%%%%%%\n";
    // Display the black list.
    for (auto& test_name : blacklist) {
      cout << "\t\t" << test_name << "\n";
    }
  }

  bool run_only = false;
  size_t num_total = all_tests.size();
  if (uargs.size() != 0) {
    run_only = true;
    num_total = 0;
    if (!need_only_one) {
      cout << "****** RunTests: command line present, running only:\n";
    }
    for (size_t i = 0; i < uargs.size(); i++) {
      if (!found_in_table(uargs[i].c_str())) {
        cout << "****** ERROR! cmd_line test not found - " << uargs[i] << "\n";
        return -3;
      }
      cout << "\t" << uargs[i] << "\n";
      num_total += 1;
    }
    //cout << "\n";
  }

  if (cmd_line.GetNamedArgs().size()) {
    cout << "******* ARGS TO TESTS:\n";
    for (auto& p : cmd_line.GetNamedArgs()) {
      cout << "\t" << p.first << " => " << p.second << "\n";
    }
    //cout << "\n";
  }

  for (uint32_t ntest=0; ntest < test_repeat_val; ntest++) {

    size_t nskipped = 0;

    cout << "******* RUNNING " << num_total << " TESTS...\n";
    for (auto test : all_tests) {
      // The test might not be present in command line for inclusive list
      if (run_only && !found_in_cmd_line(test.func_name)) {
        if (!need_only_one) {
          //cout << "*** SKIPPING test (not included) - " << test.func_name << ".\n";
        }
        nskipped += 1;
        continue;
      }
      // Or test also can be blacklisted
      if (std::find(blacklist.begin(), blacklist.end(), test.func_name) != blacklist.end()) {
        //cout << "*** SKIPPING test - IT'S BLACKLISTED - " << test.func_name << ".\n";
        nskipped += 1;
        continue;
      }
      
      if (nskipped != 0) {
        cout << "*** SKIPPED " << nskipped << " tests\n";
      }

      if (test_repeat_found) {
        cout << "*** Running test " << test.func_name << " (ntest/test_repeat " << ntest << "/" << test_repeat_val << ") ...\n";
      }
      else {
        cout << "*** Running test " << test.func_name << "\n";
      }
      /*bool done_ok = false;
      try {*/

        test.lpfn_test_func(info);

      /*  done_ok = true;
      }
      catch (boost::system::system_error& e) {
        cout << "<==== Exception boost system_error: " << e.what() << "\n";
        PrintBoostDiagnosticInformation(e, test.func_name, cout);
      }
      catch (std::system_error& e) {
        cout << "<==== Exception std::system_error: " << e.what() << "\n";
        PrintBoostDiagnosticInformation(e, test.func_name, cout);
      }
      catch (std::runtime_error& e) {
        cout << "<==== Exception std::runtime_error: " << e.what() << "\n";
        PrintBoostDiagnosticInformation(e, test.func_name, cout);
      }
      catch (std::exception& e) {
        cout << "<==== :-( Exception std::exception: " << e.what() << "\n";
        PrintBoostDiagnosticInformation(e, test.func_name, cout);
      }
      DCHECK(done_ok);
      */
      cout << "\n";
    }
  }

  if (!info.opts_dict.empty()) {
    // Some tests may not run therefore cmdline may contain args, it's OK
    cout << "Args which are not consumed by tests:\n";
    KeyedCmdLine<char>::PrintUnknownArgsLeft(cout, info.opts_dict, "\t");
  }

  cout << "\n"
       << "~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~\n"
       << "All good.\n"
       << "\n"
       << "You got all your tests just working.\n"
       << "~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~\n\n";

  return 0;
}

}



