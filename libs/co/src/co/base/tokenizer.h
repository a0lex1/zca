#pragma once

#include "co/common.h"

namespace co {

class TokenizerBase {
public:
  virtual ~TokenizerBase() = default;

  TokenizerBase(const char* token_splitter) : token_splitter_(token_splitter) {}

  /* Main function to read tokens */
  virtual bool GetNextToken(std::string& token) = 0;

  template <class PushbackableContainer>
  bool GetAllTokens(PushbackableContainer& container) {
    std::string token;
    bool ret = false;
    bool cleared = false;
    while (GetNextToken(token)) {
      if (!cleared) {
        container.clear();
        cleared = true;
      }
      container.push_back(token);
      ret = true; // at least one
    }
    return ret;
  }

protected:
  const char* GetTokenSplitter() const { return token_splitter_.c_str(); }

private:
  std::string token_splitter_;
};

// --

// simple string tokenizer
class Tokenizer : public TokenizerBase {
public:
  virtual ~Tokenizer() = default;

  Tokenizer(const char* splitter, const void* user_buf, size_t user_buf_len);

  virtual bool GetNextToken(std::string& token) override;

private:
  const char* user_buf_;
  size_t user_buf_len_;
  size_t cur_pos_;
};


}


