#include "co/base/buffered_tokenizer.h"
#include "co/base/strings.h"

using namespace std;

namespace co {

BufferedTokenizer::BufferedTokenizer(size_t max_size, const char* token_splitter)
  :
  TokenizerBase(token_splitter),
  buf_(max_size),
  cur_pos_(0)
{
}

char* BufferedTokenizer::GetBufferSpace(size_t& bytes_left)
{
  bytes_left = buf_.size() - cur_pos_;
  if (bytes_left == 0) {
    // dont go outside vector's range
    static char empty_str[] = "\0";
    return empty_str;
  }
  return &buf_[cur_pos_];
}

void BufferedTokenizer::OnBufferSpaceFilled(size_t num_bytes)
{
  cur_pos_ += num_bytes;
}

size_t BufferedTokenizer::SpaceLeft() const
{
  return buf_.size() - cur_pos_;
}

bool BufferedTokenizer::GetNextToken(string& token)
{
  if (buf_.empty()) {
    return false;
  }
  char* splitter_ptr = strnstr(static_cast<const char*>(&buf_[0]),
                               GetTokenSplitter(),
                               cur_pos_);
  if (splitter_ptr == nullptr) {
    return false;
  }
  size_t splitter_pos = splitter_ptr - &buf_[0];
  token.clear();
  token.append(&buf_[0], splitter_pos);
  // erase (move to beginning)
  // can't use memcpy cuz it moves dwords/qwords, they can overlap
  size_t from_start = 0;
  // splitter_pos+1 cuz erase \n too
  for (size_t i = splitter_pos + 1; i < cur_pos_; i++) {
    buf_[from_start++] = buf_[i];
  }
  cur_pos_ -= splitter_pos + 1;
#ifndef NDEBUG
  memset(&buf_[cur_pos_], '\0', buf_.size() - cur_pos_);
#endif
  return true;
}


}

