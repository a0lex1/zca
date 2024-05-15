// From github (old)
#pragma once

#include "co/base/strings.h"

namespace co {

static const unsigned kRadixHex16 = 16; // putting this in global namespace was not clever

class HexDump {
public:
  enum section_set {
    eDigits                    = 1,
    eDigitsThanText            = 2,
    eAddressThanDigits         = 3,
    eAddressThanDigitsThanText = 4,
    eAddressThanText           = 5,
    eTextOnly                  = 6
  };
  enum section_flags {
    eAddressSection    = 1 << 0,
    eDigitsSection     = 1 << 1,
    eTextSection       = 1 << 2
  };

  enum row_type {
    eByte,
    eWord,
    eDword,
    eQword
  };

  HexDump(unsigned    rows_per_line  = 16,
               section_set sec_set        = eDigitsThanText,
               bool        widechar_text  = false,
               row_type    _row_type      = eByte,
               unsigned    radix          = kRadixHex16,
               bool        leading_zeros  = true);

  HexDump& RowsPerLine(unsigned rpl);
  HexDump& SectionSet(section_set sset);

  void DoHexDumpW(std::wstring& append_to, const void* display_addr, const void* mem, size_t mem_len) const;
  std::wstring DoHexDumpW(const void* display_addr, const void* mem, size_t mem_len) const;
  std::wstring DoHexDumpW(const void* display_addr, const std::string&) const;

  void DoHexDump(std::string& append_to, const void* display_addr, const void* mem, size_t mem_len) const;
  std::string DoHexDump(const void* display_addr, const void* mem, size_t mem_len) const;
  std::string DoHexDump(const void* display_addr, const std::string&) const;

private:
  void assert_params_valid() const;
  size_t row_size() const;
  size_t row_max_len() const;
  bool address_section_enabled() const;
  bool digits_section_enabled() const;
  bool text_section_enabled() const;

private:
  unsigned rows_per_line_;
  row_type row_type_;
  unsigned radix_;
  section_set sec_set_;
  bool leading_zeros_;
  bool widechar_text_;
};

}
