#pragma once

#include "co/base/tokenizer.h"

namespace co {

// tokenizer where you can input chunks and read lexems (can overflow, see |max_size|)
class BufferedTokenizer : public TokenizerBase {
public:
  virtual ~BufferedTokenizer() = default;

  BufferedTokenizer(size_t max_size, const char* token_splitter);

  char* GetBufferSpace(size_t& bytes_left);
  void OnBufferSpaceFilled(size_t num_bytes);
  size_t SpaceLeft() const;

  // Tokenizer impl.
  virtual bool GetNextToken(std::string& token) override;

private:
  std::vector<char> buf_;
  size_t cur_pos_;
};


}

