#pragma once

#include "co/base/tokenizer.h"

namespace netshell {
namespace detail {

class StringReader {
public:
  virtual ~StringReader() = default;

  virtual bool ReadString(std::string& buf) = 0;
};

template <typename Cont>
class StringReaderFromContainer : public StringReader {
public:
  StringReaderFromContainer(const Cont& cont) : cont_(cont), it_(cont_.begin()) {}

  bool ReadString(std::string& buf) override {
    if (it_ == cont_.end()) {
      return false;
    }
    buf = *it_;
    it_++;
    return true;
  }
private:
  Cont cont_;
  typename Cont::const_iterator it_;
};

class StringReaderFromBuffer : public StringReader {
public:
  StringReaderFromBuffer(const void* buf, size_t buf_len)
    :
    tok_("\n", buf, buf_len)
  {

  }
  bool ReadString(std::string& buf) override {
    return tok_.GetNextToken(buf);
  }
private:
  co::Tokenizer tok_;
};


}}


