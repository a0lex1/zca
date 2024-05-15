#pragma once

#include "co/base/strings.h"
#include "co/base/find_string_index.h"

#include <string.h>

namespace core {

namespace detail {
enum class Op {
  eEQ,
  eCaseInsensitiveEQ,
  eWildcardEQ,
  eWildcardCaseInsensitiveEQ
};
struct Record {
  std::string side_a;
  Op op;
  std::string side_b;

  Record(const std::string& _side_a, Op _op, const std::string& _side_b): side_a(_side_a), op(_op), side_b(_side_b) {}
};
}

// -i ip~127.* -e ver~3
class BotFilter {
public:
  bool AddWhitelist(const std::string& subj) {
    return AddToList(whitelist_records_, subj);
  }

  bool AddBlacklist(const std::string& subj) {
    return AddToList(blacklist_records_, subj);
  }

  void SetHeaderRow(const StringVector& hdr_row) {
    hdr_row_ = &hdr_row;
  }

  // Returns true if filter is *PASSED*
  bool ApplyFilterToRow(const StringVector& row) {
    DCHECK(hdr_row_->size() == row.size());
    if (whitelist_records_.size()) {
      return FindMatch(whitelist_records_, row);
    }
    if (blacklist_records_.size()) {
      return !FindMatch(blacklist_records_, row);
    }
    return true;
  }

private:
  bool FindMatch(const std::vector<detail::Record>& records,
    const StringVector& row)
  {
    for (size_t nrec = 0; nrec < records.size(); nrec++) {
      const detail::Record& rec(records[nrec]);
      size_t key_index = co::FindStringIndex(*hdr_row_, rec.side_a);
      if (key_index == -1) {
        continue;
      }
      const std::string& actual_value(row[key_index]);

      if (rec.op == detail::Op::eEQ) {
        if (actual_value == rec.side_b) {
          return true;
        }
      }
      else if (rec.op == detail::Op::eCaseInsensitiveEQ) {
        if (!StrICmp(actual_value.c_str(), rec.side_b.c_str())) {
          return true;
        }
      }
      else {
        NOTREACHED();
      }
    }
    return false;
  }

  bool AddToList(std::vector<detail::Record>& records, const std::string& subj) {
    StringVector parts;
    if (strchr(subj.c_str(), '=')) {
      co::string_split(subj, "=", parts);
      records.emplace_back(detail::Record(parts[0], detail::Op::eEQ, parts[1]));
      return true;
    }
    else if (strchr(subj.c_str(), '%')) {
      co::string_split(subj, "%", parts);
      records.emplace_back(detail::Record(parts[0], detail::Op::eCaseInsensitiveEQ, parts[1]));
      return true;
    }
    else if (strchr(subj.c_str(), '~')) {
      co::string_split(subj, "~", parts);
      records.emplace_back(detail::Record(parts[0], detail::Op::eWildcardEQ, parts[1]));
      return true;
    }
    else if (strchr(subj.c_str(), '$')) {
      co::string_split(subj, "$", parts);
      records.emplace_back(detail::Record(parts[0], detail::Op::eWildcardCaseInsensitiveEQ, parts[1]));
      return true;
    }
    else {
      return false;
    }
  }

private:
  std::vector<detail::Record> whitelist_records_;
  std::vector<detail::Record> blacklist_records_;
  const StringVector* hdr_row_{ nullptr };
};

}


