#pragma once

#include "vlanbleed2/async/test_kit/stm/script.h"
#include "co/async/service.h"

#include "co/base/dict.h"

class ServiceWithScriptsTester {
public:
  using Service = co::async::Service;

  void TestServiceWithScripts(StringMap& opts_dict, co::ConsumeAction consume,
                              bool dry_run, const std::string& link_addr_str,
                              Service& service,
                              const std::vector<Script>& participants);

  void DisplayScriptReport(const std::vector<SOp>& ops,
                           const std::vector<SOpRes>& op_results,
                           bool print_header,
                           const std::string& name);

};

