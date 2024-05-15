#include "co/base/tests.h"
#include "co/base/config.h"
#include "co/base/convert_value.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;

enum SampleType { eSampleTiger, eSampleCrocodile, eSampleDog };

struct SampleConfig {
  SampleType sample_type;
  uint32_t sample_size;

  SampleConfig(SampleType _sample_type = eSampleTiger,
    uint32_t _sample_size = 0)
    :
    sample_type(_sample_type), sample_size(_sample_size)
  {
  }
};

static SampleConfig GetDefaultLogConfig() {
  return SampleConfig(eSampleCrocodile, 777);
}

/*GCC told it's unused*/
static bool ConvertValue(const string& str, SampleType& val) {
  if (str == "tiger") {
    val = eSampleTiger;
  } else {
    if (str == "crocodile") {
      val = eSampleCrocodile;
    } else {
      if (str == "dog") {
        val = eSampleDog;
      } else {
        return false;
      }
    }
  }
  return true;
}

// ------------- without exception

class SampleConfigFromDictNoexcept: public ConfigFromDictNoexcept<SampleConfig, string, string> {
public:
  virtual ~SampleConfigFromDictNoexcept() = default;

  using ConfigType = SampleConfig;

  SampleConfigFromDictNoexcept() {}
  SampleConfigFromDictNoexcept(const SampleConfig& default_config,
    StringMap& dict, ConsumeAction consume_action, const vector<string>& required_fields = {})
    :
    ConfigFromDictNoexcept(default_config, dict, consume_action, required_fields)
  {
    Parse();
  }
private:
  void Parse() {
    if (!OverrideFromConfigField<uint32_t>("sample-size", sample_size)) {
      return;
    }
    if (!OverrideFromConfigField<SampleType>("sample-type", sample_type)) {
      return;
    }
  }
private:
};

// ------------- with exception

class SampleConfigFromDict : public ConfigFromDict<SampleConfigFromDictNoexcept, string, string> {
public:
  using ConfigFromDict::ConfigFromDict;

  virtual ~SampleConfigFromDict() = default;
};

/*
  SampleConfigFromDictNoexcept::SampleConfigFromDictNoexcept(
    const SampleConfig& default_config,
    StringMap& dict,
    ConsumeAction consume_action,
    const vector<string>& required_fields = {})

  with/without exceptions
  with/without consume
  with/without required
  opt_parsing_failed/required_opt_not_found

// w/o exceptions, w/o consume, w/o required, OK
// w/o exceptions, w/o consume, missing opt, OK
// w/o exceptions, w/o consume, w/o required, opt_parsing_failed
// w/o exceptions, w/o consume, w required, required_opt_not_found
// w/o exceptions, w consume, w/o required, OK
// //w/o exceptions, w consume, w required, required_opt_not_found <- can't know what already consumed!
  ---
// w exceptions, w/o consume, w/o required, OK
// w exceptions, w/o consume, missing opt, OK
// w exceptions, w/o consume, w/o required, opt_parsing_failed
// w exceptions, w/o consume, w required, required_opt_not_found
// w exceptions, w consume, w/o required, OK
// //w exceptions, w consume, w required, required_opt_not_found <- can't know what already consumed!
*/

void test_co_config(TestInfo&) {
  StringMap dict;
  SampleConfigFromDictNoexcept lc;
  // w/o exceptions, w/o consume, w/o required, OK
  dict = { {"sample-type", "dog"}, { "sample-size", "1234" } };
  lc = SampleConfigFromDictNoexcept(GetDefaultLogConfig(), dict, ConsumeAction::kDontConsume);
  DCHECK(!lc.GetError());
  DCHECK(lc.sample_size == 1234);
  DCHECK(lc.sample_type == eSampleDog);
  DCHECK(dict.size() == 2); // nothing consumed
  // w/o exceptions, w/o consume, w/o required, missing opt, OK
  dict = { {"sample-type", "dog"} }; // sample-size is missing but not required
  lc = SampleConfigFromDictNoexcept(GetDefaultLogConfig(), dict, ConsumeAction::kDontConsume, {"sample-type"});
  DCHECK(!lc.GetError());
  DCHECK(lc.sample_size == GetDefaultLogConfig().sample_size);
  DCHECK(lc.sample_type == eSampleDog);
  DCHECK(dict.size() == 1); // nothing consumed
  // w/o exceptions, w/o consume, w/o required, opt_parsing_failed
  dict = { {"sample-type", "tiger"}, { "sample-size", "xxxyouyyy" } };
  lc = SampleConfigFromDictNoexcept(GetDefaultLogConfig(), dict, ConsumeAction::kDontConsume);
  syslog(_INFO) << "Errcode " << lc.GetError() << "\n";
  DCHECK(lc.GetError().GetErrc() == ConfigErrc::dict_error);
  DCHECK(lc.GetError().GetErrorInfo().dict_error.GetErrc() == DictErrc::opt_parsing_failed);
  DCHECK(lc.GetError().GetErrorInfo().dict_error.GetErrorInfo().problem_key == "sample-size");
  DCHECK(lc.GetError().GetErrorInfo().dict_error.GetErrorInfo().problem_value == "xxxyouyyy");
  // w/o exceptions, w/o consume, w required, required_opt_not_found
  dict = { {"sample-type", "crocodile"} };
  lc = SampleConfigFromDictNoexcept(GetDefaultLogConfig(), dict, ConsumeAction::kDontConsume, {"sample-size"});
  syslog(_INFO) << "Errcode " << lc.GetError() << "\n";
  DCHECK(lc.GetError().GetErrc() == ConfigErrc::dict_error);
  DCHECK(lc.GetError().GetErrorInfo().dict_error.GetErrc() == DictErrc::required_opt_not_found);
  DCHECK(lc.GetError().GetErrorInfo().dict_error.GetErrorInfo().problem_key == "sample-size");
  DCHECK(lc.GetError().GetErrorInfo().dict_error.GetErrorInfo().problem_value == "");
  // w/o exceptions, w consume, w/o required, OK
  dict = { {"sample-type", "dog"}, { "sample-size", "1234" } };
  lc = SampleConfigFromDictNoexcept(GetDefaultLogConfig(), dict, ConsumeAction::kConsume);
  DCHECK(!lc.GetError());
  DCHECK(lc.sample_size == 1234);
  DCHECK(lc.sample_type == eSampleDog);
  DCHECK(dict.size() == 0); // all consumed
  // ---------------- exceptions -----------------
  // w exceptions, w/o consume, w/o required, OK
  try {
    dict = { {"sample-type", "dog"}, { "sample-size", "1234" } };
    lc = SampleConfigFromDict(GetDefaultLogConfig(), dict, ConsumeAction::kDontConsume);
    DCHECK(!lc.GetError());
    DCHECK(lc.sample_size == 1234);
    DCHECK(lc.sample_type == eSampleDog);
    DCHECK(dict.size() == 2); // nothing consumed
  }
  catch (ConfigException& e) {
    (void)e;
    NOTREACHED();
  }
  // w exceptions, w/o consume, missing opt, OK
  try {
    dict = { {"sample-type", "dog"} }; // sample-size is missing but not required
    lc = SampleConfigFromDict(GetDefaultLogConfig(), dict, ConsumeAction::kDontConsume, { "sample-type" });
    DCHECK(!lc.GetError());
    DCHECK(lc.sample_size == GetDefaultLogConfig().sample_size);
    DCHECK(lc.sample_type == eSampleDog);
    DCHECK(dict.size() == 1); // nothing consumed
  }
  catch (ConfigException& e) {
    (void)e;
    NOTREACHED();
  }
  // w exceptions, w/o consume, w/o required, opt_parsing_failed
  try {
    dict = { {"sample-type", "tiger"}, { "sample-size", "xxxyouyyy" } };
    lc = SampleConfigFromDict(GetDefaultLogConfig(), dict, ConsumeAction::kDontConsume);
    NOTREACHED();
  }
  catch (ConfigException& e) {
    DCHECK(e.GetError().GetErrc() == ConfigErrc::dict_error);
    DCHECK(e.GetError().GetErrorInfo().dict_error.GetErrc() == DictErrc::opt_parsing_failed);
    DCHECK(e.GetError().GetErrorInfo().dict_error.GetErrorInfo().problem_key == "sample-size");
    DCHECK(e.GetError().GetErrorInfo().dict_error.GetErrorInfo().problem_value == "xxxyouyyy");
  }
  // w exceptions, w/o consume, w required, required_opt_not_found
  try {
    dict = { {"sample-type", "crocodile"} };
    lc = SampleConfigFromDict(GetDefaultLogConfig(), dict, ConsumeAction::kDontConsume, { "sample-size" });
    NOTREACHED();
  }
  catch (ConfigException& e) {
    DCHECK(e.GetError().GetErrc() == ConfigErrc::dict_error);
    DCHECK(e.GetError().GetErrorInfo().dict_error.GetErrc() == DictErrc::required_opt_not_found);
    DCHECK(e.GetError().GetErrorInfo().dict_error.GetErrorInfo().problem_key == "sample-size");
    DCHECK(e.GetError().GetErrorInfo().dict_error.GetErrorInfo().problem_value == "");
    syslog(_INFO) << "OK, expected Config exception: " << e.what() << "\n";
  }
  // w exceptions, w consume, w/o required, OK
  try {
    dict = { {"sample-type", "dog"}, { "sample-size", "1234" } };
    lc = SampleConfigFromDict(GetDefaultLogConfig(), dict, ConsumeAction::kConsume);
    DCHECK(!lc.GetError());
    DCHECK(lc.sample_size == 1234);
    DCHECK(lc.sample_type == eSampleDog);
    DCHECK(dict.size() == 0); // all consumed
  }
  catch (ConfigException& e) {
    (void)e;
    NOTREACHED();
  }
}























